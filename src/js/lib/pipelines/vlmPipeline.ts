// Copyright (C) 2025 Intel Corporation
// SPDX-License-Identifier: Apache-2.0

import util from "node:util";
import { VLMPipeline as VLMPipelineWrap } from "../addon.js";
import { GenerationConfig, StreamingStatus, VLMPipelineProperties } from "../utils.js";
import { Tokenizer } from "../tokenizer.js";
import type { Tensor } from "openvino-node";

export type ResolveFunction = (arg: { value: string; done: boolean }) => void;
export type Options = {
  disableStreamer?: boolean;
  max_new_tokens?: number;
};

/** Structure with raw performance metrics for VLM generation. */
export type VLMRawMetrics = {
  /** Durations for embedding preparation in milliseconds. */
  prepareEmbeddingsDurations: number[];
};

/** Structure with raw performance metrics for each generation before any statistics are calculated. */
export type RawMetrics = {
  /** Durations for each generate call in milliseconds. */
  generateDurations: number[];
  /** Durations for the tokenization process in milliseconds. */
  tokenizationDurations: number[];
  /** Durations for the detokenization process in milliseconds. */
  detokenizationDurations: number[];
  /** Times to the first token for each call in milliseconds. */
  timesToFirstToken: number[];
  /** Timestamps of generation every token or batch of tokens in milliseconds. */
  newTokenTimes: number[];
  /** Inference time for each token in milliseconds. */
  tokenInferDurations: number[];
  /** Batch sizes for each generate call. */
  batchSizes: number[];
  /** Total durations for each generate call in milliseconds. */
  durations: number[];
  /** Total inference duration for each generate call in microseconds. */
  inferenceDurations: number[];
  /** Time to compile the grammar in milliseconds. */
  grammarCompileTimes: number[];
};

/** Structure holding mean and standard deviation values. */
export type MeanStdPair = {
  mean: number;
  std: number;
};

/** Structure holding summary of statistical values */
export type SummaryStats = {
  mean: number;
  std: number;
  min: number;
  max: number;
};

/**
 * Holds performance metrics for each VLM generate call.
 *
 * VLMPerfMetrics extends PerfMetrics with VLM-specific metrics:
    - Prepare embeddings duration, ms
 */
export interface VLMPerfMetrics {
  /** Returns the load time in milliseconds. */
  getLoadTime(): number;
  /** Returns the number of generated tokens. */
  getNumGeneratedTokens(): number;
  /** Returns the number of tokens in the input prompt. */
  getNumInputTokens(): number;
  /** Returns the mean and standard deviation of Time To the First Token (TTFT) in milliseconds. */
  getTTFT(): MeanStdPair;
  /** Returns the mean and standard deviation of Time Per Output Token (TPOT) in milliseconds. */
  getTPOT(): MeanStdPair;
  /** Returns the mean and standard deviation of Inference time Per Output Token in milliseconds. */
  getIPOT(): MeanStdPair;
  /** Returns the mean and standard deviation of throughput in tokens per second. */
  getThroughput(): MeanStdPair;
  /** Returns the mean and standard deviation of the time spent on model inference during generate call in milliseconds. */
  getInferenceDuration(): MeanStdPair;
  /** Returns the mean and standard deviation of generate durations in milliseconds. */
  getGenerateDuration(): MeanStdPair;
  /** Returns the mean and standard deviation of tokenization durations in milliseconds. */
  getTokenizationDuration(): MeanStdPair;
  /** Returns the mean and standard deviation of detokenization durations in milliseconds. */
  getDetokenizationDuration(): MeanStdPair;
  /** Returns a map with the time to initialize the grammar compiler for each backend in milliseconds. */
  getGrammarCompilerInitTimes(): { [key: string]: number };
  /** Returns the mean, standard deviation, min, and max of grammar compile times in milliseconds. */
  getGrammarCompileTime(): SummaryStats;
  /** Returns the mean and standard deviation of embeddings preparation duration in milliseconds. */
  getPrepareEmbeddingsDuration(): MeanStdPair;
  /** A structure of RawPerfMetrics type that holds raw metrics. */
  rawMetrics: RawMetrics;
  /** VLM specific raw metrics */
  vlmRawMetrics: VLMRawMetrics;

  /** Adds the metrics from another VLMPerfMetrics object to this one.
   * @returns The current VLMPerfMetrics instance.
   */
  add(other: VLMPerfMetrics): this;
}

export class VLMDecodedResults {
  constructor(texts: string[], scores: number[], perfMetrics: VLMPerfMetrics) {
    this.texts = texts;
    this.scores = scores;
    this.perfMetrics = perfMetrics;
  }
  toString() {
    if (this.scores.length !== this.texts.length) {
      throw new Error("The number of scores and texts doesn't match in VLMDecodedResults.");
    }
    if (this.texts.length === 0) {
      return "";
    }
    if (this.texts.length === 1) {
      return this.texts[0];
    }
    let result = "";
    for (let i = 0; i < this.texts.length - 1; ++i) {
      result += `${this.scores[i].toFixed(6)}: ${this.texts[i]}\n`;
    }
    result += `${this.scores[this.scores.length - 1].toFixed(
      6,
    )}: ${this.texts[this.texts.length - 1]}`;

    return result;
  }
  texts: string[];
  scores: number[];
  perfMetrics: VLMPerfMetrics;
}

export class VLMPipeline {
  modelPath: string;
  device: string;
  pipeline: any | null = null;
  properties: VLMPipelineProperties;
  isInitialized = false;
  isChatStarted = false;

  constructor(modelPath: string, device: string, properties: VLMPipelineProperties) {
    this.modelPath = modelPath;
    this.device = device;
    this.properties = properties;
  }

  async init() {
    if (this.isInitialized) throw new Error("VLMPipeline is already initialized");

    this.pipeline = new VLMPipelineWrap();

    const initPromise = util.promisify(this.pipeline.init.bind(this.pipeline));
    const result = await initPromise(this.modelPath, this.device, this.properties);

    this.isInitialized = true;

    return result;
  }

  async startChat(systemMessage: string = "") {
    if (this.isChatStarted) throw new Error("Chat is already started");

    const startChatPromise = util.promisify(this.pipeline.startChat.bind(this.pipeline));
    const result = await startChatPromise(systemMessage);

    this.isChatStarted = true;

    return result;
  }

  async finishChat() {
    if (!this.isChatStarted) throw new Error("Chat is not started");

    const finishChatPromise = util.promisify(this.pipeline.finishChat.bind(this.pipeline));
    const result = await finishChatPromise();

    this.isChatStarted = false;

    return result;
  }

  stream(
    prompt: string,
    images: Tensor[] = [],
    videos: Tensor[] = [],
    generationConfig: GenerationConfig = {},
  ) {
    if (!this.isInitialized) throw new Error("Pipeline is not initialized");

    if (typeof generationConfig !== "object") throw new Error("Options must be an object");

    let streamingStatus: StreamingStatus = StreamingStatus.RUNNING;
    const queue: { isDone: boolean; subword: string }[] = [];
    let resolvePromise: ResolveFunction | null;

    // Callback function that C++ will call when a chunk is ready
    function chunkOutput(isDone: boolean, subword: string) {
      if (resolvePromise) {
        // Fulfill pending request
        resolvePromise({ value: subword, done: isDone });
        resolvePromise = null; // Reset promise resolver
      } else {
        // Add data to queue if no pending promise
        queue.push({ isDone, subword });
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
          const { isDone, subword } = data;

          return { value: subword, done: isDone };
        }

        return new Promise((resolve: ResolveFunction) => (resolvePromise = resolve));
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

    return new Promise((resolve: (value: VLMDecodedResults) => void) => {
      const chunkOutput = (isDone: boolean, result: string | any) => {
        if (isDone) {
          const decodedResults = new VLMDecodedResults(
            result.texts,
            result.scores,
            result.perfMetrics,
          );
          resolve(decodedResults);
        } else if (callback && typeof result === "string") {
          return callback(result);
        }

        return StreamingStatus.RUNNING;
      };
      this.pipeline.generate(prompt, images, videos, chunkOutput, generationConfig, options);
    });
  }

  getTokenizer(): Tokenizer {
    return this.pipeline.getTokenizer();
  }

  setChatTemplate(chatTemplate: string): void {
    if (!this.isInitialized) throw new Error("Pipeline is not initialized");
    this.pipeline.setChatTemplate(chatTemplate);
  }

  getGenerationConfig(): GenerationConfig {
    if (!this.isInitialized) throw new Error("Pipeline is not initialized");
    return this.pipeline.getGenerationConfig();
  }

  setGenerationConfig(config: GenerationConfig): void {
    if (!this.isInitialized) throw new Error("Pipeline is not initialized");
    this.pipeline.setGenerationConfig(config);
  }
}
