import { dowloadModel } from './utils.js';
import { models } from './models.js';

for (const model of Object.values(models)) {
  await dowloadModel(model);
}
