# Reference tool
# given a bmp, it will determine which color palette indices have been used
# this tool can be used to properly implement slp parsing
# usage: ref.py palette bmp1 bmp2...

import matplotlib.pyplot as plt
import numpy as np
import sys
import os


def main():
	if len(sys.argv) < 3:
		print('usage: ref.py palette bmp1 bmp2...')
		sys.exit(1)

	pal_path = sys.argv[1]
	bmp_paths = sys.argv[2:]

	print(f'palette path: {pal_path}, bmps: {bmp_paths}')

	pal = plt.imread(pal_path)
	pal_w, pal_h = 16, 16
	dim = (pal_w, pal_h, 3)
	if pal.shape != dim:
		raise ValueError(f'invalid palette: expected {dim}, got {pal.shape}')

	#plt.imshow(pal)
	#plt.show()

	cols = {}

	for y in range(pal_h):
		for x in range(pal_w):
			r, g, b = pal[y, x]

			item = (r, g, b)
			# duplicates can occur in e.g. the game main palette. ignore them as we don't need them
			if item in cols:
				continue

			cols[(r, g, b)] = y * pal_w + x

	# add white and black with dummy index
	item = (0, 0, 0)
	if item not in cols:
		cols[item] = -1

	item = (255, 255, 255)
	if item not in cols:
		cols[item] = -1

	#print(cols)

	# now parse the images and find the corresponding color palette indices
	for path in bmp_paths:
		ref = plt.imread(path)

		h, w, b = ref.shape
		if b != 3 and b != 4:
			raise ValueError(f'unsupported bit depth: {b}')

		decoded = np.zeros((h,w), dtype=np.uint8)

		for y in range(h):
			for x in range(w):
				decoded[y, x] = cols[tuple(ref[y, x, :3])]

		txt_path = f'{os.path.splitext(path)[0]}.txt'
		print(f'saving pixels to {txt_path}')
		np.savetxt(txt_path, decoded, fmt='%3d')


if __name__ == '__main__':
	main()
