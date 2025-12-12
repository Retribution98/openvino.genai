// Copyright (C) 2025 Intel Corporation
// SPDX-License-Identifier: Apache-2.0

import util from "node:util";
import { VLMPipeline as VLMPipelineWrapper } from "../addon.js";
import { GenerationConfig, VLMPipelineProperties, StreamingStatus } from "../utils.js";
import { VLMDecodedResults } from "../decodedResults.js";
import { Tokenizer } from "../tokenizer.js";
import type { Tensor } from "openvino-node";
import { VLMPerfMetrics } from "../perfMetrics.js";

/**
 * This class is used for generation with Visual Language Models (VLMs)
 */
export class VLMPipeline {
  modelPath: string;
  device: string;
  pipeline: VLMPipelineWrapper | null = null;
  properties: VLMPipelineProperties;

  /**
   * VLMPipeline class constructor.
   * @param {string|os.PathLike} modelPath - Path to the folder with exported model files or model identifier.
   * @param {string} device - Device to run the model on (e.g., 'CPU', 'GPU').
   * @param {VLMPipelineProperties} properties - Device and pipeline properties.
   */
  constructor(modelPath: string, device: string, properties: VLMPipelineProperties) {
    this.modelPath = modelPath;
    this.device = device;
    this.properties = properties;
  }

  /**
   * Initialize the underlying native pipeline.
   * @returns {Promise<void>} Resolves when initialization is complete.
   */
  async init() {
    const pipeline = new VLMPipelineWrapper();

    const initPromise = util.promisify(pipeline.init.bind(pipeline));
    await initPromise(this.modelPath, this.device, this.properties);

    this.pipeline = pipeline;
  }
  /**
   * Start a chat session with an optional system message.
   * @param {string} [systemMessage=''] - Optional system message to initialize chat context.
   * @returns {Promise<void>} Resolves when chat session is started.
   */
  async startChat(systemMessage: string = "") {
    if (!this.pipeline) throw new Error("Pipeline is not initialized");

    const startChatPromise = util.promisify(this.pipeline.startChat.bind(this.pipeline));
    const result = await startChatPromise(systemMessage);

    return result;
  }
  /**
   * Finish the current chat session and clear chat-related state.
   * @returns {Promise<void>} Resolves when chat session is finished.
   */
  async finishChat() {
    if (!this.pipeline) throw new Error("Pipeline is not initialized");

    const finishChatPromise = util.promisify(this.pipeline.finishChat.bind(this.pipeline));
    const result = await finishChatPromise();

    return result;
  }
  /**
   * Stream generation results as an async iterator of strings.
   * The iterator yields subword chunks.
   * @param {string} prompt - Input prompt. May contain image/video tags recognized by the model.
   * @param {object} [options] - Optional parameters.
   * @param {Tensor[]} [options.images] - Array of image tensors to include in the prompt.
   * @param {Tensor[]} [options.videos] - Array of video frame tensors to include in the prompt.
   * @param {GenerationConfig} [options.generationConfig] - Generation parameters.
   * @returns {AsyncIterableIterator<string>} Async iterator producing subword chunks.
   */
  stream(
    prompt: string,
    options?: {
      images?: Tensor[];
      videos?: Tensor[];
      generationConfig?: GenerationConfig;
    },
  ): AsyncIterableIterator<string> {
    if (!this.pipeline) throw new Error("Pipeline is not initialized");
    const { images = [], videos = [], generationConfig = {} } = options || {};

    let streamingStatus: StreamingStatus = StreamingStatus.RUNNING;
    const queue: { done: boolean; subword: string }[] = [];
    type ResolveFunction = (arg: { value: string; done: boolean }) => void;
    type RejectFunction = (reason?: unknown) => void;
    let resolvePromise: ResolveFunction | null;
    let rejectPromise: RejectFunction | null;

    const callback = (
      error: Error | null,
      result: { texts: string[]; scores: number[]; perfMetrics: VLMPerfMetrics },
    ) => {
      if (error) {
        if (rejectPromise) {
          rejectPromise(error);
          // Reset promises
          resolvePromise = null;
          rejectPromise = null;
        } else {
          throw error;
        }
      } else {
        const decodedResult = new VLMDecodedResults(
          result.texts,
          result.scores,
          result.perfMetrics,
        );
        const fullText = decodedResult.toString();
        if (resolvePromise) {
          // Fulfill pending request
          resolvePromise({ done: true, value: fullText });
          // Reset promises
          resolvePromise = null;
          rejectPromise = null;
        } else {
          // Add data to queue if no pending promise
          queue.push({ done: true, subword: fullText });
        }
      }
    };

    const streamer = (chunk: string): StreamingStatus => {
      if (resolvePromise) {
        // Fulfill pending request
        resolvePromise({ done: false, value: chunk });
        // Reset promises
        resolvePromise = null;
        rejectPromise = null;
      } else {
        // Add data to queue if no pending promise
        queue.push({ done: false, subword: chunk });
      }
      return streamingStatus;
    };

    this.pipeline.generate(callback, prompt, images, videos, streamer, generationConfig);

    return {
      async next() {
        // If there is data in the queue, return it
        // Otherwise, return a promise that will resolve when data is available
        const data = queue.shift();

        if (data !== undefined) {
          const { done, subword } = data;

          return { value: subword, done: done };
        }

        return new Promise((resolve: ResolveFunction, reject: (reason?: unknown) => void) => {
          resolvePromise = resolve;
          rejectPromise = reject;
        });
      },
      async return() {
        streamingStatus = StreamingStatus.CANCEL;

        return { done: true, value: "" };
      },
      [Symbol.asyncIterator]() {
        return this;
      },
    };
  }
  /**
   * Generate sequences for VLMs.
   * @param {string} prompt - Input prompt. May contain model-specific image/video tags.
   * @param {object} [options] - Optional parameters.
   * @param {Tensor[]} [options.images] - Images to include in the prompt.
   * @param {Tensor[]} [options.videos] - Videos to include in the prompt.
   * @param {GenerationConfig} [options.generationConfig] - Generation configuration parameters.
   * @param {(chunk: string) => void} [options.streamer] - Optional streamer callback called for each chunk.
   * @returns {Promise<VLMDecodedResults>} Resolves with decoded results once generation finishes.
   */
  async generate(
    prompt: string,
    options?: {
      images?: Tensor[];
      videos?: Tensor[];
      generationConfig?: GenerationConfig;
      streamer?: (chunk: string) => StreamingStatus;
    },
  ): Promise<VLMDecodedResults> {
    const { images = [], videos = [], generationConfig = {}, streamer } = options || {};

    return new Promise(
      (resolve: (value: VLMDecodedResults) => void, reject: (reason?: unknown) => void) => {
        const callback = (
          error: Error | null,
          result: { texts: string[]; scores: number[]; perfMetrics: VLMPerfMetrics },
        ) => {
          if (error) {
            reject(error);
          } else {
            resolve(new VLMDecodedResults(result.texts, result.scores, result.perfMetrics));
          }
        };
        if (!this.pipeline) {
          reject(new Error("Pipeline is not initialized"));
        } else {
          this.pipeline.generate(callback, prompt, images, videos, streamer, generationConfig);
        }
      },
    );
  }

  /**
   * Get the pipeline tokenizer instance.
   * @returns {Tokenizer} Tokenizer used by the pipeline.
   */
  getTokenizer(): Tokenizer {
    if (!this.pipeline) throw new Error("Pipeline is not initialized");
    return this.pipeline.getTokenizer();
  }

  /**
   * Set the chat template used when formatting chat history and prompts.
   * @param {string} chatTemplate - Chat template string.
   */
  setChatTemplate(chatTemplate: string): void {
    if (!this.pipeline) throw new Error("Pipeline is not initialized");
    this.pipeline.setChatTemplate(chatTemplate);
  }

  /**
   * Set generation configuration parameters.
   * @param {GenerationConfig} config - Generation configuration parameters.
   */
  setGenerationConfig(config: GenerationConfig): void {
    if (!this.pipeline) throw new Error("Pipeline is not initialized");
    this.pipeline.setGenerationConfig(config);
  }
}
