# Copyright 2018-2025 Admenri.
# Use of this source code is governed by a BSD - style license that can be
# found in the LICENSE file.

import sys
import os
import bindgen as bgen

if __name__ == "__main__":
  in_dir = sys.argv[1]
  out_dir = sys.argv[2]

  template_classes = []
  for filepath in os.listdir(in_dir):
    with open(in_dir + filepath, 'r', encoding='utf-8') as file:
      file_content = file.read()

      idl_parser = bgen.api_parser.APIParser()
      idl_parser.parse(file_content)
      for klass in idl_parser.classes:
        template_classes.append(klass)

  os.makedirs(out_dir, exist_ok=True)

  for filepath in os.listdir(out_dir):
    os.remove(out_dir + filepath)

  for klass in template_classes:
    generator = bgen.MriBindGen()
    generator.setup(klass)

    with open(out_dir + generator.file_name + ".h", "w", encoding="utf-8") as f:
      f.write(generator.gen_header())
    with open(out_dir + generator.file_name + ".cc", "w", encoding="utf-8") as f:
      f.write(generator.gen_source())
