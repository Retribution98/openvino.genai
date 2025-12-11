// Copyright (C) 2025 Intel Corporation
// SPDX-License-Identifier: Apache-2.0

import { PerfMetrics, VLMPerfMetrics } from "./perfMetrics.js";

export class DecodedResults {
  constructor(texts: string[], scores: number[], perfMetrics: PerfMetrics) {
    this.texts = texts;
    this.scores = scores;
    this.perfMetrics = perfMetrics;
  }
  toString() {
    if (this.scores.length !== this.texts.length) {
      throw new Error("The number of scores and texts doesn't match in DecodedResults.");
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
  perfMetrics: PerfMetrics;
}

export class VLMDecodedResults extends DecodedResults {
  constructor(texts: string[], scores: number[], perfMetrics: VLMPerfMetrics) {
    super(texts, scores, perfMetrics);
    this.perfMetrics = perfMetrics;
  }

  perfMetrics: VLMPerfMetrics;
}
