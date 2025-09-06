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
	shutil.rmtree(www_dir_path)
	gzip_dir_content(static_build_path, www_dir_path)

env.AddPreAction('buildfs', pre_gzip_static)
