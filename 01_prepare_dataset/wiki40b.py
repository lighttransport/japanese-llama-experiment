import tensorflow_datasets as tfds

(ds_train, ds_valid, ds_test), ds_info = tfds.load(
  name='wiki40b/ja', split=['train', 'validation', 'test'], with_info=True)
