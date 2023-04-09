# Tool that shows and saves color palettes
# usage: palette.py palette1 palette2 ...

import matplotlib.pyplot as plt
import sys
import struct
import numpy as np
import os


ROWS = 256
JASC_PAL = f'JASC-PAL\r\n0100\r\n{ROWS}\r\n'


def main():
	if len(sys.argv) < 2:
		print(f'usage: program palettes...')
		sys.exit(1)

	for path in sys.argv[1:]:
		try:
			with open(path, 'r', newline='\r\n') as f:
				lines = f.readlines()
				magic = ''.join(lines[0:3])

				if magic != JASC_PAL:
					raise ValueError(f'{path}: incorrect header')

				palette = np.zeros((ROWS, 3), dtype=np.uint8)

				for i in range(ROWS):
					r, g, b = map(int, lines[3+i].split(' '))
					palette[i] = [r, g, b]

				w = 16
				h = ROWS // w

				img = palette.reshape((h,w,3))

				bmp_path = f'{os.path.splitext(path)[0]}.bmp'
				print(f'saving to {bmp_path}')
				plt.imsave(bmp_path, img)

				plt.imshow(img)
				plt.show()
		except UnicodeDecodeError:
			raise ValueError(f'{path}: bad palette')


if __name__ == '__main__':
	main()
