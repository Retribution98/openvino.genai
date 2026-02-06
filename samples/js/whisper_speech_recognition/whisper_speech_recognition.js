// Copyright (C) 2025 Intel Corporation
// SPDX-License-Identifier: Apache-2.0

import { spawn } from 'node:child_process';
import { readFileSync } from 'node:fs';
import { basename } from 'node:path';
import ffmpegPath from 'ffmpeg-static';
import yargs from 'yargs/yargs';
import { hideBin } from 'yargs/helpers';
import { WhisperPipeline } from 'openvino-genai-node';

const TARGET_SAMPLE_RATE = 16000;

/**
 * Run a command and return stdout as a Buffer.
 * @param {string} command
 * @param {string[]} args
 * @returns {Promise<Buffer>}
 */
function spawnCapture(command, args) {
  return new Promise((resolve, reject) => {
    const child = spawn(command, args, { stdio: ['ignore', 'pipe', 'pipe'] });
    const chunks = [];
    let stderr = '';
    child.stdout?.on('data', (d) => chunks.push(d));
    child.stderr?.on('data', (d) => { stderr += d.toString(); });
    child.on('error', reject);
    child.on('close', (code) => {
      if (code === 0) resolve(Buffer.concat(chunks));
      else reject(new Error(`ffmpeg exited with code ${code}: ${stderr}`));
    });
  });
}

/**
 * Read a WAV (or any audio file supported by ffmpeg) and return Float32Array
 * normalized to ~[-1, 1] at 16 kHz mono. Uses ffmpeg-static when available.
 * @param {string} filepath - Path to the audio file
 * @returns {Promise<Float32Array>}
 */
async function readWavWithFfmpeg(filepath) {
  if (!ffmpegPath) {
    throw new Error('ffmpeg-static not available; use readWav() for PCM WAV only');
  }
  const stdout = await spawnCapture(ffmpegPath, [
    '-v', 'error',
    '-i', filepath,
    '-ar', String(TARGET_SAMPLE_RATE),
    '-ac', '1',
    '-f', 'f32le',
    '-',
  ]);
  const numSamples = stdout.length / 4;
  return new Float32Array(stdout.buffer, stdout.byteOffset, numSamples);
}

/**
 * Read a PCM WAV file and return Float32Array of samples normalized to ~[-1, 1] at 16 kHz.
 * Supports 16-bit PCM mono or stereo (stereo is mixed to mono). Resamples to 16 kHz if needed.
 * Use readWavWithFfmpeg() for any audio format (via ffmpeg-static).
 * @param {string} filepath - Path to the WAV file
 * @returns {Float32Array}
 */
function readWav(filepath) {
  const buffer = readFileSync(filepath);
  let offset = 0;

  if (buffer.toString('ascii', 0, 4) !== 'RIFF') {
    throw new Error('Not a valid WAV file (missing RIFF header)');
  }
  offset += 8; // RIFF + size

  if (buffer.toString('ascii', offset, offset + 4) !== 'WAVE') {
    throw new Error('Not a valid WAV file (missing WAVE)');
  }
  offset += 4;

  let sampleRate = TARGET_SAMPLE_RATE;
  let numChannels = 1;
  let bitsPerSample = 16;
  let dataOffset = 0;
  let dataSize = 0;

  while (offset < buffer.length - 8) {
    const chunkId = buffer.toString('ascii', offset, offset + 4);
    const chunkSize = buffer.readUInt32LE(offset + 4);
    offset += 8;

    if (chunkId === 'fmt ') {
      const audioFormat = buffer.readUInt16LE(offset);
      if (audioFormat !== 1) {
        throw new Error('Only PCM WAV (format 1) is supported');
      }
      numChannels = buffer.readUInt16LE(offset + 2);
      sampleRate = buffer.readUInt32LE(offset + 4);
      bitsPerSample = buffer.readUInt16LE(offset + 14);
    } else if (chunkId === 'data') {
      dataOffset = offset;
      dataSize = chunkSize;
    }
    offset += chunkSize;
  }

  if (dataSize === 0) {
    throw new Error('WAV file has no data chunk');
  }

  const numSamples = dataSize / (numChannels * (bitsPerSample / 8));
  const samples = new Float32Array(numSamples);

  for (let i = 0; i < numSamples; i++) {
    let sum = 0;
    for (let c = 0; c < numChannels; c++) {
      const idx = (i * numChannels + c) * 2;
      const s = buffer.readInt16LE(dataOffset + idx);
      sum += s;
    }
    samples[i] = (sum / numChannels) / 32768;
  }

  if (sampleRate !== TARGET_SAMPLE_RATE) {
    const ratio = sampleRate / TARGET_SAMPLE_RATE;
    const newLength = Math.round(numSamples / ratio);
    const resampled = new Float32Array(newLength);
    for (let i = 0; i < newLength; i++) {
      const srcIdx = i * ratio;
      const i0 = Math.floor(srcIdx);
      const i1 = Math.min(i0 + 1, numSamples - 1);
      const t = srcIdx - i0;
      resampled[i] = samples[i0] * (1 - t) + samples[i1] * t;
    }
    return resampled;
  }

  return samples;
}

function getConfigForCache() {
  const config = { CACHE_DIR: 'whisper_cache' };
  return config;
}

async function main() {
  const argv = yargs(hideBin(process.argv))
    .scriptName(basename(process.argv[1]))
    .command(
      '$0 <model_dir> <audio_file> [device]',
      'Run Whisper speech recognition on an audio file',
      (yargsBuilder) =>
        yargsBuilder
          .positional('model_dir', {
            type: 'string',
            describe: 'Path to the converted Whisper model directory',
            demandOption: true,
          })
          .positional('audio_file', {
            type: 'string',
            describe: 'Path to the audio file (WAV, MP3, M4A, etc.; decoded via ffmpeg to 16 kHz mono)',
            demandOption: true,
          })
          .positional('device', {
            type: 'string',
            describe: 'Device to run the model on (e.g. CPU, GPU)',
            default: 'CPU',
          }),
    )
    .strict()
    .help()
    .parse();

  const modelDir = argv.model_dir;
  const wavFilePath = argv.audio_file;
  const device = argv.device;

  let properties = {};
  if (device === 'NPU' || device.startsWith('GPU')) {
    properties = getConfigForCache();
  }
  // Word timestamps require word_timestamps in the pipeline constructor
  properties.word_timestamps = true;

  const pipeline = await WhisperPipeline(modelDir, device, properties);

  const config = pipeline.getGenerationConfig();
  const generationConfig = {
    ...config,
    language: '<|en|>',
    task: 'transcribe',
    return_timestamps: true,
    word_timestamps: true,
  };

  const rawSpeech = await readWavWithFfmpeg(wavFilePath);
  const result = await pipeline.generate(rawSpeech, { generationConfig });

  console.log(result.texts?.[0] ?? '');

  if (result.chunks?.length) {
    for (const chunk of result.chunks) {
      console.log(`timestamps: [${chunk.startTs.toFixed(2)}, ${chunk.endTs.toFixed(2)}] text: ${chunk.text}`);
    }
  }

  if (result.words?.length) {
    for (const word of result.words) {
      console.log(`[${word.startTs.toFixed(2)}, ${word.endTs.toFixed(2)}]: ${word.word}`);
    }
  }
}

main().catch((err) => {
  console.error(err);
  process.exit(1);
});
