// Copyright (C) 2025 Intel Corporation
// SPDX-License-Identifier: Apache-2.0

import util from "node:util";
import { VLMPipeline as VLMPipelineWrapper } from "../addon.js";
import {
  GenerationConfig,
  StreamingStatus,
  VLMPipelineProperties,
  ResolveFunction,
  RejectFunction,
} from "../utils.js";
import { VLMDecodedResults } from "../decodedResults.js";
import { Tokenizer } from "../tokenizer.js";
import type { Tensor } from "openvino-node";
import { VLMPerfMetrics } from "../perfMetrics.js";

export class VLMPipeline {
  modelPath: string;
  device: string;
  pipeline: VLMPipelineWrapper | null = null;
  properties: VLMPipelineProperties;

  constructor(modelPath: string, device: string, properties: VLMPipelineProperties) {
    this.modelPath = modelPath;
    this.device = device;
    this.properties = properties;
  }

  async init() {
    const pipeline = new VLMPipelineWrapper();

    const initPromise = util.promisify(pipeline.init.bind(pipeline));
    await initPromise(this.modelPath, this.device, this.properties);

    this.pipeline = pipeline;
  }

  async startChat(systemMessage: string = "") {
    if (!this.pipeline) throw new Error("Pipeline is not initialized");

    const startChatPromise = util.promisify(this.pipeline.startChat.bind(this.pipeline));
    const result = await startChatPromise(systemMessage);

    return result;
  }

  async finishChat() {
    if (!this.pipeline) throw new Error("Pipeline is not initialized");

    const finishChatPromise = util.promisify(this.pipeline.finishChat.bind(this.pipeline));
    const result = await finishChatPromise();

    return result;
  }

  stream(
    prompt: string,
    images: Tensor[] = [],
    videos: Tensor[] = [],
    generationConfig: GenerationConfig = {},
  ) {
    if (!this.pipeline) throw new Error("Pipeline is not initialized");

    if (typeof generationConfig !== "object") throw new Error("Options must be an object");

    let streamingStatus: StreamingStatus = StreamingStatus.RUNNING;
    const queue: { done: boolean; subword: string }[] = [];
    let resolvePromise: ResolveFunction | null;
    let rejectPromise: RejectFunction | null;

    // Callback function that C++ will call when a chunk is ready
    function chunkOutput(
      error: Error | null,
      subword: string | { texts: string[]; scores: number[]; perfMetrics: VLMPerfMetrics },
    ) {
      if (error) {
        if (rejectPromise) {
          rejectPromise(error);
          // Reset promise
          resolvePromise = null;
          rejectPromise = null;
        } else {
          throw error;
        }
      } else {
        let done = false;
        if (typeof subword !== "string") {
          done = true;
          subword = new VLMDecodedResults(
            subword.texts,
            subword.scores,
            subword.perfMetrics,
          ).toString();
        }
        if (resolvePromise) {
          // Fulfill pending request
          resolvePromise({ value: subword.toString(), done });
          resolvePromise = null; // Reset promise resolver
          rejectPromise = null;
        } else {
          // Add data to queue if no pending promise
          queue.push({ done, subword: subword.toString() });
        }
      }

      return streamingStatus;
    }

    this.pipeline.generate(prompt, images, videos, chunkOutput, generationConfig, {});

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

        return { done: true };
      },
      [Symbol.asyncIterator]() {
        return this;
      },
    };
  }

  async generate(
    prompt: string,
    images: Tensor[] = [],
    videos: Tensor[] = [],
    generationConfig: GenerationConfig = {},
    callback?: (chunk: string) => void,
  ): Promise<VLMDecodedResults> {
    if (typeof generationConfig !== "object") throw new Error("Options must be an object");
    if (callback !== undefined && typeof callback !== "function")
      throw new Error("Callback must be a function");

    const options: { disableStreamer?: boolean } = {};
    if (!callback) {
      options["disableStreamer"] = true;
    }

    return new Promise(
      (resolve: (value: VLMDecodedResults) => void, reject: (reason?: unknown) => void) => {
        const chunkOutput = (
          error: Error | null,
          result: string | { texts: string[]; scores: number[]; perfMetrics: VLMPerfMetrics },
        ) => {
          if (error) {
            reject(error);
          } else {
            if (typeof result !== "string") {
              resolve(new VLMDecodedResults(result.texts, result.scores, result.perfMetrics));
            } else if (callback) {
              try {
                return callback(result);
              } catch (err) {
                // If the user callback throws an error, add stack trace information and rethrow
                if (err instanceof Error && err.stack) {
                  err.message += `\n${err.stack}`;
                }
                // We should rethrow the error instead of rejecting the promise
                // to finish ThreadSafeFunction correctly
                throw err;
              }
            }
          }

          return StreamingStatus.RUNNING;
        };
        if (!this.pipeline) {
          reject(new Error("Pipeline is not initialized"));
        } else {
          this.pipeline.generate(prompt, images, videos, chunkOutput, generationConfig, options);
        }
      },
    );
  }

  getTokenizer(): Tokenizer {
    if (!this.pipeline) throw new Error("Pipeline is not initialized");
    return this.pipeline.getTokenizer();
  }

  setChatTemplate(chatTemplate: string): void {
    if (!this.pipeline) throw new Error("Pipeline is not initialized");
    this.pipeline.setChatTemplate(chatTemplate);
  }

  setGenerationConfig(config: GenerationConfig): void {
    if (!this.pipeline) throw new Error("Pipeline is not initialized");
    this.pipeline.setGenerationConfig(config);
  }
}
