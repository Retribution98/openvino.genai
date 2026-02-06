// Copyright (C) 2025 Intel Corporation
// SPDX-License-Identifier: Apache-2.0

import { describe, it, before } from "node:test";
import assert from "node:assert/strict";
import { resolve, dirname } from "node:path";
import { fileURLToPath } from "node:url";
import { WhisperPipeline, StreamingStatus } from "../dist/index.js";
import { WhisperPipeline as WhisperPipelineClass } from "../dist/pipelines/whisperPipeline.js";

const WHISPER_MODEL_PATH =
  process.env.WHISPER_MODEL_PATH || resolve("/opt/home/ksuvorov/git/openvino.genai/ov_cache/test_models", "whisper-tiny-int8");

/**
 * Creates raw speech buffer for tests.
 * @param {Object} [options]
 * @param {number} [options.durationSeconds=0.1] - Duration in seconds
 * @param {number} [options.sampleRate=16000] - Sample rate in Hz
 * @param {number} [options.fillValue=0] - Fill value (e.g. 0 for silence), normalized to [-1, 1]
 * @returns {Float32Array}
 */
function createTestRawSpeech(options = {}) {
  const { durationSeconds = 0.1, sampleRate = 16000, fillValue = 0 } = options;
  const length = Math.round(durationSeconds * sampleRate);
  return new Float32Array(length).fill(fillValue);
}

describe("WhisperPipeline creation", () => {
  it("WhisperPipeline is exported from index", () => {
    assert.strictEqual(typeof WhisperPipeline, "function");
  });

  it("WhisperPipeline(modelPath, device) creates and initializes pipeline", async () => {
    const pipeline = await WhisperPipeline(WHISPER_MODEL_PATH, "CPU");
    assert.ok(pipeline);
    assert.strictEqual(typeof pipeline.generate, "function");
    assert.strictEqual(typeof pipeline.getTokenizer, "function");
    assert.strictEqual(typeof pipeline.getGenerationConfig, "function");
    assert.strictEqual(typeof pipeline.setGenerationConfig, "function");
  });

  it("WhisperPipeline(modelPath, device, properties) accepts optional properties", async () => {
    const pipeline = await WhisperPipeline(WHISPER_MODEL_PATH, "CPU", {});
    assert.ok(pipeline);
  });
});

describe("WhisperPipeline methods", () => {
  let pipeline;
  let rawSpeech;

  before(async () => {
    pipeline = await WhisperPipeline(WHISPER_MODEL_PATH, "CPU");
    rawSpeech = createTestRawSpeech();
  });

  it("generate(rawSpeech) returns texts, scores and perfMetrics", async () => {
    const result = await pipeline.generate(rawSpeech);
    assert.ok(Array.isArray(result.texts));
    assert.ok(Array.isArray(result.scores));
    assert.strictEqual(result.texts.length, result.scores.length);
    assert.ok(result.perfMetrics);
    assert.strictEqual(typeof result.perfMetrics.getLoadTime, "function");
    assert.strictEqual(typeof result.perfMetrics.getTTFT, "function");
    assert.strictEqual(typeof result.perfMetrics.getTPOT, "function");
  });

  it("generate(rawSpeech, options) accepts generationConfig", async () => {
    const result = await pipeline.generate(rawSpeech, {
      generationConfig: { language: "<|en|>", task: "transcribe" },
    });
    assert.ok(Array.isArray(result.texts));
    assert.ok(result.perfMetrics);
  });

  it("generate(rawSpeech, options) with streamer calls streamer with chunks", async () => {
    const chunks = [];
    const result = await pipeline.generate(rawSpeech, {
      streamer: (chunk) => {
        chunks.push(chunk);
        assert.strictEqual(typeof chunk, "string");
        return StreamingStatus.RUNNING;
      },
    });
    assert.ok(chunks.length > 0);
    assert.strictEqual(chunks.join(""), result.texts[0]);
  });

  it("stream(rawSpeech) returns async iterator of chunks", async () => {
    const chunks = [];
    const stream = pipeline.stream(rawSpeech);
    for await (const chunk of stream) {
      chunks.push(chunk);
      assert.strictEqual(typeof chunk, "string");
    }
    assert.ok(Array.isArray(chunks));
    assert.ok(chunks.length > 0);
  });

  it("stream(rawSpeech, options) accepts generation config", async () => {
    const chunks = [];
    const stream = pipeline.stream(rawSpeech, { language: "<|en|>", task: "transcribe" });
    for await (const chunk of stream) {
      chunks.push(chunk);
      assert.strictEqual(typeof chunk, "string");
    }
    assert.ok(Array.isArray(chunks));
    assert.ok(chunks.length > 0);
  });

  it("getTokenizer() returns tokenizer instance", () => {
    const tokenizer = pipeline.getTokenizer();
    assert.ok(tokenizer);
    assert.strictEqual(typeof tokenizer.encode, "function");
    assert.strictEqual(typeof tokenizer.decode, "function");
  });

  it("getGenerationConfig() returns config object", () => {
    const config = pipeline.getGenerationConfig();
    assert.ok(config && typeof config === "object");
    assert.strictEqual(typeof config.return_timestamps, "boolean");
    assert.strictEqual(typeof config.max_new_tokens, "number");
  });

  it("setGenerationConfig(config) updates config", () => {
    const newConfig = { language: "<|en|>", task: "transcribe" };
    pipeline.setGenerationConfig(newConfig);
    const config = pipeline.getGenerationConfig();
    assert.strictEqual(config.language, newConfig.language);
    assert.strictEqual(config.task, newConfig.task);
  });

  it("throws when generate() is called before init()", async () => {
    const uninitializedPipeline = new WhisperPipelineClass("/nonexistent", "CPU");
    await assert.rejects(
      uninitializedPipeline.generate(new Float32Array(100)),
      /WhisperPipeline is not initialized/,
    );
  });
});

describe("WhisperPipeline with word_timestamps=true", () => {
  let pipeline;
  let rawSpeech;

  before(async () => {
    pipeline = await WhisperPipeline(WHISPER_MODEL_PATH, "CPU", { word_timestamps: true });
    rawSpeech = createTestRawSpeech();
  });

  it("getGenerationConfig() returns word_timestamps: true", () => {
    const config = pipeline.getGenerationConfig();
    assert.strictEqual(config.word_timestamps, true);
  });

  it("generate() without generationConfig doesn't returns chunks but returns words", async () => {
    const result = await pipeline.generate(rawSpeech);
    assert.strictEqual(result.chunks, undefined);
    assert.ok(Array.isArray(result.words));
    for (const w of result.words) {
      assert.strictEqual(typeof w.word, "string");
      assert.strictEqual(typeof w.startTs, "number");
      assert.strictEqual(typeof w.endTs, "number");
      assert.ok(Array.isArray(w.tokenIds));
      assert.ok(w.tokenIds.every((id) => typeof id === "number"));
    }
  });

  it("generate() with return_timestamps returns chunks with timestamps", async () => {
    const result = await pipeline.generate(rawSpeech, {
      generationConfig: { return_timestamps: true },
    });
    assert.ok(result.chunks && result.chunks.length > 0);
    for (const chunk of result.chunks) {
      assert.strictEqual(typeof chunk.text, "string");
      assert.strictEqual(typeof chunk.startTs, "number");
      assert.strictEqual(typeof chunk.endTs, "number");
    }
  });
});

describe("WhisperPerfMetrics", () => {
  let pipeline;
  let rawSpeech;

  before(async () => {
    pipeline = await WhisperPipeline(WHISPER_MODEL_PATH, "CPU");
    rawSpeech = createTestRawSpeech();
  });

  it("result.perfMetrics has base getters and optionally Whisper-specific ones", async () => {
    const result = await pipeline.generate(rawSpeech);
    const pm = result.perfMetrics;
    assert.ok(pm);
    assert.strictEqual(typeof pm.getLoadTime, "function");
    assert.strictEqual(typeof pm.getTTFT, "function");
    if (typeof pm.getFeaturesExtractionDuration === "function") {
      assert.strictEqual(typeof pm.getWordLevelTimestampsProcessingDuration, "function");
      assert.ok("whisperRawMetrics" in pm);
      const raw = pm.whisperRawMetrics;
      assert.ok(raw);
      assert.ok(Array.isArray(raw.featuresExtractionDurations));
      assert.ok(Array.isArray(raw.wordLevelTimestampsProcessingDurations));
    }
  });

  it("getFeaturesExtractionDuration() returns MeanStdPair when available", async () => {
    const result = await pipeline.generate(rawSpeech);
    if (typeof result.perfMetrics.getFeaturesExtractionDuration !== "function") {
      return;
    }
    const pair = result.perfMetrics.getFeaturesExtractionDuration();
    assert.ok(pair && typeof pair === "object");
    assert.strictEqual(typeof pair.mean, "number");
    assert.strictEqual(typeof pair.std, "number");
  });

  it("getWordLevelTimestampsProcessingDuration() returns MeanStdPair when available", async () => {
    const result = await pipeline.generate(rawSpeech);
    if (typeof result.perfMetrics.getWordLevelTimestampsProcessingDuration !== "function") {
      return;
    }
    const pair = result.perfMetrics.getWordLevelTimestampsProcessingDuration();
    assert.ok(pair && typeof pair === "object");
    assert.strictEqual(typeof pair.mean, "number");
    assert.strictEqual(typeof pair.std, "number");
  });
});
