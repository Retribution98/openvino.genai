// Copyright (C) 2025 Intel Corporation
// SPDX-License-Identifier: Apache-2.0

import { Tokenizer, VLMPipeline } from "../dist/index.js";

import assert from "node:assert";
import { describe, it, before } from "node:test";
import { promises as fs } from "node:fs";
import { models } from "./models.js";
import { createTestImageTensor, createTestVideoTensor } from "./utils.js";

const MODEL_PATH = process.env.VLM_MODEL_PATH || `./tests/models/${models.VLLM.split("/")[1]}`;

describe("VLMPipeline", () => {
  let pipeline = null;

  before(async () => {
    try {
      await fs.access(MODEL_PATH);
    } catch {
      console.log(`Model not found at ${MODEL_PATH}, skipping VLM tests`);
      return;
    }

    pipeline = await VLMPipeline(MODEL_PATH, "CPU");
    pipeline.setGenerationConfig({ max_new_tokens: 10 });
  });

  it("should generate text without images", async () => {
    if (!pipeline) return;

    const result = await pipeline.generate("What is 2+2?");

    assert.ok(result.texts.length > 0, "Should generate some output");
  });

  it("should generate text with images", async () => {
    if (!pipeline) return;

    const testImage1 = createTestImageTensor();
    const testImage2 = createTestImageTensor();
    const result = await pipeline.generate("Compare these two images.", [testImage1, testImage2]);

    assert.strictEqual(result.texts.length, 1, "Should generate comparison");
  });

  it("should generate text with video input", async () => {
    if (!pipeline) return;

    const testVideo = createTestVideoTensor();

    const result = await pipeline.generate(
      "Describe what happens in this video.",
      [],
      [testVideo],
      {
        max_new_tokens: 20,
        temperature: 0,
      },
    );

    assert.strictEqual(result.texts.length, 1);
  });

  it("should generate with both image and video", async () => {
    if (!pipeline) return;

    const testImage = createTestImageTensor();
    const testVideo = createTestVideoTensor();

    const result = await pipeline.generate(
      "Compare the image and video.",
      [testImage],
      [testVideo],
      { max_new_tokens: 20, temperature: 0 },
    );

    assert.strictEqual(result.texts.length, 1);
  });

  it("should support streaming generation", async () => {
    if (!pipeline) return;

    const testImage = createTestImageTensor();
    const chunks = [];

    const stream = pipeline.stream("What do you see?", [testImage], [], {
      max_new_tokens: 15,
      temperature: 0,
    });

    for await (const chunk of stream) {
      chunks.push(chunk);
    }

    assert.ok(chunks.length > 0, "Should receive streaming chunks");
    const fullOutput = chunks.join("");
    assert.ok(fullOutput.length > 0, "Combined chunks should form output");
  });

  it("should return VLMDecodedResults with perfMetrics", async () => {
    if (!pipeline) return;

    const testImage = createTestImageTensor();
    const result = await pipeline.generate("Describe the image.", [testImage], [], {
      max_new_tokens: 10,
      temperature: 0,
    });

    assert.ok(result, "Should return result");
    assert.ok(result.perfMetrics, "Should have perfMetrics");
    // Property frome base PerformanceMetrics
    const numTokens = result.perfMetrics.getNumGeneratedTokens();
    assert.ok(typeof numTokens === "number", "getNumGeneratedTokens should return number");
    assert.ok(numTokens > 0, "Should generate at least one token");
    // VLM-specific properties
    const prepareEmbeddings = result.perfMetrics.getPrepareEmbeddingsDuration();
    assert.ok(
      typeof prepareEmbeddings.mean === "number",
      "PrepareEmbeddingsDuration should have mean",
    );
    const { prepareEmbeddingsDurations } = result.perfMetrics.vlmRawMetrics;
    assert.ok(
      Array.isArray(prepareEmbeddingsDurations),
      "Should have duration of preparation of embeddings",
    );
    assert.ok(prepareEmbeddingsDurations.length > 0, "Should have at least one duration value");
  });

  it("should get tokenizer from pipeline", () => {
    if (!pipeline) return;
    const tokenizer = pipeline.getTokenizer();
    assert.ok(tokenizer instanceof Tokenizer, "Should return tokenizer");
  });

  it("should start and finish chat", async () => {
    if (!pipeline) return;

    await pipeline.startChat("You are a helpful assistant.");
    assert.ok(pipeline.isChatStarted, "Chat should be started");

    await pipeline.finishChat();
    assert.ok(!pipeline.isChatStarted, "Chat should be finished");
  });
});
