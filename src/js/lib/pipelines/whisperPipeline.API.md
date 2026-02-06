# WhisperPipeline: сравнение JS API с Python API

## Реализовано и совпадает с Python

| Элемент | Python | JS |
|--------|--------|-----|
| **Конструктор** | `WhisperPipeline(models_path, device, **kwargs)` | `new WhisperPipeline(modelPath, device, properties)` + `init()` (асинхронная инициализация) |
| **generate** | `generate(raw_speech, generation_config=None, streamer=None, **kwargs)` | `generate(rawSpeech, { generationConfig?, streamer? })` → `Promise<WhisperDecodedResults>` |
| **stream** | Через `streamer` в `generate()` | `stream(rawSpeech, options?)` → `AsyncIterableIterator<string>` (удобная обёртка) |
| **get_tokenizer** | `get_tokenizer()` | `getTokenizer()` |
| **get_generation_config** | `get_generation_config()` | `getGenerationConfig()` |
| **set_generation_config** | `set_generation_config(config)` | `setGenerationConfig(config)` |
| **Результат: texts, scores, chunks** | `result.texts`, `result.scores`, `result.chunks` | `result.texts`, `result.scores`, `result.chunks` ✓ |
| **Результат: words** | `result.words` (list[WhisperWordTiming]: word, start_ts, end_ts, token_ids) | `result.words` (WhisperWordTiming[]: word, startTs, endTs, tokenIds?) ✓ |
| **Результат: perf_metrics** | `result.perf_metrics` (WhisperPerfMetrics) | `result.perfMetrics` (WhisperPerfMetrics) ✓ |
| **WhisperPerfMetrics** | `get_features_extraction_duration()`, `get_word_level_timestamps_processing_duration()`, `whisper_raw_metrics` | `getFeaturesExtractionDuration()`, `getWordLevelTimestampsProcessingDuration()`, `whisperRawMetrics` ✓ |
| **Конфиг: language, task, return_timestamps, word_timestamps, initial_prompt, hotwords** | ✓ | ✓ в `WhisperGenerationConfig` (utils.ts) |
| **Конфиг: расширенные поля** | `alignment_heads`, `decoder_start_token_id`, `pad_token_id`, и др. | ✓ в `WhisperGenerationConfig`: `alignment_heads`, `decoder_start_token_id`, `pad_token_id`, `no_timestamps_token_id`, `transcribe_token_id`, `translate_token_id`, `prev_sot_token_id`, `lang_to_id`, `max_initial_timestamp_index`, `is_multilingual`, `begin_suppress_tokens`, `suppress_tokens` |
| **Свойства пайплайна** | `word_timestamps`, `ENABLE_MMAP`, и др. через `**kwargs` | `properties: WhisperPipelineProperties` (в т.ч. `word_timestamps`) |

---

## Отличия и пробелы

### 1. **generate(..., **kwargs) в Python**

- **Python:** можно вызывать `generate(raw_speech, language="<|en|>", task="transcribe")` — именованные аргументы сливаются с конфигом.
- **JS:** все опции генерации передаются только через `options.generationConfig`; отдельного «разворачивания» kwargs в конфиг нет.

**Оценка:** допустимое упрощение; для JS явный объект `generationConfig` привычнее.

---

### 2. **Передача сложных типов конфига (alignment_heads)**

- **Python:** `alignment_heads` — список пар `(layer_index, head_index)`.
- **JS:** тип `WhisperGenerationConfig` объявляет `alignment_heads?: [number, number][]`, но при передаче в нативный слой вложенные массивы могут обрабатываться не как пары чисел (ограничение конвертации JS → AnyMap). Простые поля (`pad_token_id`, `is_multilingual`, `max_initial_timestamp_index` и т.д.) передаются корректно.

---

## Итог

- **Пайплайн и основной сценарий (generate/stream, конфиг, tokenizer, chunks, words, WhisperPerfMetrics, расширенный WhisperGenerationConfig):** реализованы и соответствуют Python.
- **Оставшиеся отличия:**
  1. **kwargs**-стиль вызова `generate()` в Python — в JS опции только через `options.generationConfig` (допустимое упрощение).
  2. **alignment_heads** и другие вложенные структуры в конфиге могут требовать особой обработки при передаче в нативный код.
