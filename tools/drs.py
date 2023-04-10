import argparse
import struct


def unpack(path):
	with open(path, 'rb') as f:
		# parse header
		fmt = '<40s16sII'
		copy, version, list_count, list_end = struct.unpack(fmt, f.read(struct.calcsize(fmt)))

		print(f'copyright info: {copy}')
		print(f'version: {version}')

		print(f'list count: {list_count}, list end: {hex(list_end)}')

		groups = []

		# dump all groups
		for i in range(list_count):
			fmt = '<4sII'
			m_type, offset, size = struct.unpack(fmt, f.read(struct.calcsize(fmt)))
			m_type = m_type[::-1]

			print(f'type: {m_type.decode("utf-8")}, size: {size:3d}, offset: {hex(offset)}')

			groups += [(m_type, size, offset)]

		#print(groups)
		items = {}

		# dump all items
		for g in groups:
			m_type, size, offset = g
			f.seek(offset)

			for i in range(size):
				fmt = '<3I'
				i_id, i_offset, i_size = struct.unpack(fmt, f.read(struct.calcsize(fmt)))

				print(f'{m_type.decode("utf-8")}: {i_id:5d}, size: {hex(i_size):8s}, offset: {hex(i_offset)}')

				if i_id in items:
					raise KeyError(f'duplicate key: {i_id}')

				items[i_id] = (m_type, i_size, i_offset)

		# now extract all files
		for i_id, item in items.items():
			m_type, i_size, i_offset = item
			path = f'{i_id:05d}.{m_type.decode("utf-8").strip()}'
			print(f'{i_id:05d}: {path}')

			f.seek(i_offset)
			with open(path, 'wb') as out:
				out.write(f.read(i_size))


def main():
	parser = argparse.ArgumentParser(description='DRS processor')
	parser.add_argument('-u', help='unpack file(s)')

	args = vars(parser.parse_args())
	print(f'args: {args}')

	if args['u']:
		unpack(args['u'])


if __name__ == '__main__':
	main()
