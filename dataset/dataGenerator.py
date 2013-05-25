import argparse
import string
import random

colSources = []
baseIndex = 0
args = None

def change(word):
	word_list = list(word)
	for run in range(random.randint(0,3)):
		if len(word_list) <= 1: break
		change_index = random.randint(0, len(word_list)-1)
		word_list[change_index] = random.choice(string.letters)
	return "".join(word_list)

def write(table_file):
	table_file = open(table_file, 'w+')
	for row in range(args.max_lines):
		line = ""
		for source in colSources:
			line += change(source[row % min(100000, len(source))]) + '\t|'
		line = line[:-2]
		line += '\n'
		table_file.write(line)

def dequote(word):
	return word.replace("\"", "").strip()

def getSource(table_file, *columnIDs):
	for col in columnIDs:
		colSources.append([])
	
	global baseIndex
	line_number = 0
	for line in open(table_file):
		line_number += 1
		if line_number > args.max_lines:
			break
		parts = None
		if line.find('|') != -1:
			parts = map(dequote, line.split('|'))
		else:
			parts = map(dequote, line.split('\t'))

		# Source don't have enough columns, abandon tuple
		if len(parts) <= columnIDs[-1]:
			continue

		index = baseIndex
		for col in columnIDs:
			try:
				colSources[index].append(parts[col])
				index += 1
			except Exception:
				print "index = ", index, table_file, len(colSources), col, len(parts)

	baseIndex += len(columnIDs)

if __name__ == "__main__":
	parser = argparse.ArgumentParser('generate data from DBLP and FourSquareDataSet')
	parser.add_argument('--max_lines', default=50000)
	args = parser.parse_args()

	print baseIndex
	print vars(args)

	getSource('./dblp.table', 1, 2, 3, 4, 5, 6, 7)
	getSource('./knowledgeBase/FourSquareDataSet/Venues/LA/LA-Venues.txt', 1, 4, 5)
#	getSource('./knowledgeBase/FourSquareDataSet/Tips/LA/LA-Tips.txt', 3)
	write('./newData.table')

