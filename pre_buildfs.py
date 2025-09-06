Import('env')

import os
import gzip
import shutil

def gzip_file(src_path, dst_path):
	with open(src_path, 'rb') as src, gzip.open(dst_path, 'wb') as dst:
		for chunk in iter(lambda: src.read(4096), b""):
			dst.write(chunk)

def gzip_dir_content(source, target):
	if not os.path.exists(target):
		os.mkdir(target)

	for entry in os.listdir(source):
		src_file_path = os.path.join(source, entry)
		if os.path.isdir(src_file_path):
			gzip_dir_content(src_file_path, os.path.join(target, entry))
		else:
			dst_file_path = os.path.join(target, os.path.basename(entry) + '.gz')
			gzip_file(src_file_path, dst_file_path)

def pre_gzip_static(source, target, env):
	static_build_path = os.path.join(env.get('PROJECT_DIR'), 'web/dist')
	www_dir_path = os.path.join(env.get('PROJECT_DIR'), 'data', 'www')
	print('Building JS app in "web" to "web/dist"')
	env.Execute("pnpm -C web release")
	print('Compressing files from "web/dist" to "data/www" directory')
	if os.path.exists(www_dir_path):
		shutil.rmtree(www_dir_path)
	gzip_dir_content(static_build_path, www_dir_path)

# Delete littleFS image before adding this pre-action, otherwise it is not run if the image already exists.
# Using 'buildfs' as target instead of the more specific file path causes this script to run AFTER the creation of the image.
# I don't know if this is a bug or wanted behavior, and at this point I don't care.
fsbin = os.path.join(env['PROJECT_BUILD_DIR'], env['PIOENV'], 'littlefs.bin')
if (os.path.exists(fsbin)):
	os.remove(fsbin)

env.AddPreAction(fsbin, pre_gzip_static)
