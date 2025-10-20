import { LLMPipeline, StructuredOutputConfig } from "./dist/index.js";

const modelPath =
  "/opt/home/ksuvorov/git/openvino.genai/src/js/tests/models/Phi-4-mini-instruct-int4-ov";
const pipeline = await LLMPipeline(modelPath, "CPU");
const generationConfig = {
  max_new_tokens: 50,
  structured_output_config: new StructuredOutputConfig({
    json_schema: JSON.stringify({
      type: "object",
      properties: {
        name: { type: "string" },
        age: { type: "number" },
        city: { type: "string" },
      },
      required: ["name", "age", "city"],
    }),
  }),
  return_decoded_results: true,
};
const prompt = `Generate a JSON object with the following properties:
    - name: a random name
    - age: a random age between 1 and 100
    - city: a random city
    The JSON object should be in the following format:
    {
      "name": "John Doe",
      "age": 30,
      "city": "New York"
    }
    `;
const res = await pipeline.generateSync(prompt, generationConfig);
const text = res.texts[0];
console.log(text);