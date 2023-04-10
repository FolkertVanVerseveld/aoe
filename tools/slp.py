# Tool that shows slp image
# usage: slp.py palette image1 image2 ...

import matplotlib.pyplot as plt
import numpy as np
import sys
import struct
import os


supported_version = b'2.0N'
invalid_edge = -32768


def funpack(f, fmt):
	return struct.unpack(fmt, f.read(struct.calcsize(fmt)))


def bc_parse(w, bc, pos):
	row = []

	#print(w, pos)

	x = 0
	size = 0

	def next_byte(advance=1):
		nonlocal pos
		nonlocal size
		pos += advance
		size += advance
		return bc[pos]

	def next_cmd(advance=1):
		return next_byte(advance) & 0xf

	b = bc[pos]

	while b & 0xf != 0xf:

		if b == 0x4: # fill 1
			row += [next_byte()]
		elif b == 0x5: # skip 1
			row += [-1]
		elif b == 0x8: # fill 2
			row += [next_byte()]
			row += [next_byte()]
		elif b == 0x09:
			row += [-1]
			row += [-1]
		elif b == 0x0c: # fill 3
			for _ in range(3):
				row += [next_byte()]
		elif b == 0x10: # fill 4
			for _ in range(4):
				row += [next_byte()]
		elif b == 0x11: # skip 4
			for _ in range(4):
				row += [-1]
		elif b == 0x14: # fill 5
			for _ in range(5):
				row += [next_byte()]
		elif b == 0x1c: # fill 7
			for _ in range(7):
				row += [next_byte()]
		elif b == 0x28: # fill 10
			for _ in range(10):
				row += [next_byte()]
		else:
			print(f'unimplemented bc: {hex(b)}')

			# find end of row
			u = []
			x = pos

			while bc[x] & 0xf != 0xf:
				u += [bc[x]]
				x += 1

			print(f'pending bytecode: {u}')
			break

		b = next_byte()

	"""
	# refactor this, it's pretty broken :/
	while x < w:
		x += size
		b = next_byte()
		size = 0

		print(f'b:{b}')
		cmd = b & 0xf
		if cmd == 0xf:
			# next one should be 0xf
			cmd = next_cmd()
			if cmd != 0xf:
				raise ValueError(f'row does not end with close bytecode, expected 0xf, got {cmd}')

			break

		if b == 0x8:
			# two bytes
			row += [next_byte()]
			row += [next_byte()]
			print('0x08: {pos},{size}')
			continue

		print(f'unimplemented bc: {hex(bc[pos])}')
		# find end of row
		u = []

		while bc[x] & 0xf != 0xf:
			u += [bc[x]]
			x += 1

		print(f'pending bytecode: {u}')
		break
	"""

	pos += 1

	return row, pos


def main():
	if len(sys.argv) < 3:
		print('usage: slp.py palette image1 image2...')
		sys.exit(1)

	pal_path = sys.argv[1]
	img_paths = sys.argv[2:]
	print(f'palette path: {pal_path}, images: {img_paths}')

	pal = plt.imread(pal_path)
	dim = (16, 16, 3)
	if pal.shape != dim:
		raise ValueError(f'invalid palette: expected {dim}, got {pal.shape}')

	#plt.imshow(pal)
	#plt.show()

	# 'flatten' palette to simplify color lookup
	cols = pal.reshape((16*16, 3))

	for path in img_paths:
		with open(path, 'rb') as f:
			version, frame_count, comment = funpack(f, '<4si24s')
			print(f'version: {version.decode("utf-8")}, frames: {frame_count}, comment: {comment.decode("utf-8")}')

			if version != supported_version:
				print(f'unsupported version: {version}')

			cmd_table_offset, outline_table_offset, palette_offset, properties, width, height, hotspot_x, hotspot_y = funpack(f, '<4I4i')

			frames = []
			outlines = []

			for i in range(frame_count):
				img = np.zeros((height, width, 3))

				# make image white
				img[:,:,0] = 255
				img[:,:,1] = 255
				img[:,:,2] = 255

				# read contours
				f.seek(outline_table_offset)
				contours = []
				rectangular = True # determine if the image needs contours

				for y in range(height):
					left_space, right_space = funpack(f, '<2h')

					# check if row has to be skipped
					if left_space == invalid_edge or right_space == invalid_edge:
						contours += [(0, 0)]
						rectangular = False
						continue

					m = (left_space, right_space)
					line_size = width - m[0] - m[1]
					if line_size < width:
						rectangular = False

					contours += [m]

					# fill contour
					for x in range(m[0], m[0] + line_size):
						img[y, x, :] = 0

					# try parse cmd data
					#cmd_table_offset, bc = read_bc(f, cmd_table_offset)

				frames += [img]
				outlines += [contours]

				plt.imshow(img.astype(np.uint8))
				plt.show()

			# process cmd table code

			# determine cmd table size
			f.seek(0, 2)
			end = f.tell()

			cmd_start = cmd_table_offset + 4 * height
			size = end - cmd_start
			f.seek(cmd_start)

			# read all cmd code
			bc = f.read(size)
			print(f'bc size: {len(bc)}')
			#print(bc)

			bc_path = f'{os.path.splitext(path)[0]}.bc'
			print(f'saving bytecode to {bc_path}')
			with open(bc_path, 'wb') as out:
				out.write(bc)

			# reparse frames but now with bytecode
			for i, img, contour in zip(range(len(frames)), frames, outlines):
				pos = 0

				# try to load reference file so we can check if parsing is correct
				ref_path = f'{i:03d}_{os.path.splitext(path)[0]}.bmp'
				print(ref_path)

				for m, y in zip(contour, range(height)):
					if m == (0, 0):
						continue

					line_size = width - m[0] - m[1]
					oldpos = pos
					row, pos = bc_parse(line_size, bc, pos)
					if len(row) != line_size:
						print(f'{line_size}, {oldpos}, row: {row}')

					for x, idx in zip(range(m[0], m[0] + line_size), row):
						if idx >= 0:
							img[y, x] = cols[idx]
						else:
							img[y, x, :] = 255


				plt.imshow(img.astype(np.uint8))
				plt.show()


if __name__ == '__main__':
	main()
