import xml.sax
import MySQLdb;
import sys
 
stack = []
connect = MySQLdb.connect('localhost', 'hejian', 'hejian486', 'dblp')
cur = connect.cursor()
insertCount = 0

tableSet = set(["article", "inproceedings", "proceedings", "book",
				"incollection", "phdthesis", "mastersthesis", "www"])

authorMap = {}
editorMap = {}

class ContentHandler(xml.sax.ContentHandler):
	def __init__(self):
		xml.sax.ContentHandler.__init__(self)
		self.entityId = 0
		self.mdate = ""
		self.type = ""
		self.entityKey = ""
		self.title = ""
		self.content = ""
	
	def clearFields(self):
		self.journal = "NULL"
		self.number = "NULL"
		self.pages = "NULL"
		self.volume = "NULL"
		self.year = "NULL"
		self.series = "NULL"
		self.booktitle = "NULL"
		self.crossref = "NULL"
		self.url = "NULL"
		self.school = "NULL"
		self.title = "NULL"
		self.localAuthorSet = set()
		self.localEditorSet = set()

	def startElement(self, name, attrs):
		global stack
		global cur
		stack.append(name)
		self.content = ""
		if name in tableSet:
			self.entityId += 1
			self.mdate = attrs.getValue("mdate")
			self.entityKey = attrs.getValue("key")
			self.type = name
			self.clearFields()

	def endElement(self, name):
		global cur

		if name in tableSet:
			cur.execute("""INSERT INTO dblp_entity(entityId, entityKey, mdate, type, title) 
				VALUES(%s, %s, %s, %s, %s)""", (self.entityId, self.entityKey, self.mdate, self.type, self.title));
		
		if name == "article":
			cur.execute("""INSERT INTO dblp_entity_article(entityId, journal, number, pages,
				volume, year) VALUES(%s, %s, %s, %s, %s, %s)""", (self.entityId, self.journal, 
					self.number, self.pages, self.volume, self.year))

		if name == "book":
			cur.execute("""INSERT INTO dblp_entity_book(entityId, booktitle, series, 
				volume, year) VALUES(%s, %s, %s, %s, %s)""", (self.entityId, self.booktitle,
				self.series, self.volume, self.year))

		if name == "proceedings":
			cur.execute("""INSERT INTO dblp_entity_proceedings(entityId, booktitle, series, 
			volume, year) VALUES(%s, %s, %s, %s, %s)""", (self.entityId, self.booktitle,
				self.series, self.volume, self.year))

		if name == "incollection":
			cur.execute("""INSERT INTO dblp_entity_incollection(entityId, booktitle, crossref, 
				pages, year) VALUES(%s, %s, %s, %s, %s)""", (self.entityId, self.booktitle,
				self.crossref, self.pages, self.year))

		if name == "inproceedings":
			cur.execute("""INSERT INTO dblp_entity_inproceedings(entityId, booktitle, crossref, 
				pages, year) VALUES(%s, %s, %s, %s, %s)""", (self.entityId, self.booktitle,
				self.crossref, self.pages, self.year))

		if name == "www":
			cur.execute("""INSERT INTO dblp_entity_www(entityId, url, year) 
				VALUES(%s, %s, %s)""", (self.entityId, self.url, self.year))

		if name == "masterthesis":
			cur.execute("""INSERT INTO dblp_entity_msthesis(entityId, school, year) 
				VALUES(%s, %s, %s)""", (self.entityId, self.school, self.year))

		if name == "phdthesis":
			cur.execute("""INSERT INTO dblp_entity_phdthesis(entityId, school, year) 
				VALUES(%s, %s, %s)""", (self.entityId, self.school, self.year))

		global stack
		stack.pop()

		content = self.content
		content = content.encode('latin-1', 'ignore')
		if name == "journal": self.journal = content
		if name == "number": self.number = content
		if name == "pages": self.pages = content
		if name == "volume": self.volume = content
		if name == "year": self.year = content
		if name == "series": self.series = content
		if name == "booktitle": self.booktitle = content
		if name == "crossref": self.crossref = content
		if name == "url": self.url = content
		if name == "school": self.school = content
		if name == "title": self.title = content

		global authorMap
		global editorMap

		if name == "author":
			if content not in authorMap:
				authorMap[content] = len(authorMap) + 1
				cur.execute("""INSERT INTO dblp_author(authorId, author) VALUES(%s, %s)""",
					(authorMap[content], content))
			authorId = authorMap[content]
			try:
				cur.execute("""INSERT INTO dblp_entity_author(entityId, authorId) VALUES(%s, %s)""",
					(self.entityId, authorId))
			except MySQLdb.Error:
				print "Error when INSERT INTO dblp_entity_editor(entityId, editorId) VALUES({0}, {1})""".format(self.entityId, authorId);


		if name == "editor":
			if content not in editorMap:
				editorMap[content] = len(editorMap) + 1
				cur.execute("""INSERT INTO dblp_editor(editorId, editor) VALUES(%s, %s)""",
					(editorMap[content], content))

			editorId = editorMap[content]
			try:
				cur.execute("""INSERT INTO dblp_entity_editor(entityId, editorId) VALUES(%s, %s)""",
					(self.entityId, editorId))
			except MySQLdb.Error:
				print "Error when INSERT INTO dblp_entity_editor(entityId, editorId) VALUES({0}, {1})""".format(self.entityId, editorId);

		global connect
		global insertCount
		insertCount += 1
		if insertCount % 1000 == 0:
			print "Commit {0}".format(insertCount);
			connect.commit();

	def characters(self, content):
		self.content = self.content + content
 
def main(sourceFileName):
  source = open(sourceFileName)
  xml.sax.parse(source, ContentHandler())
 
if __name__ == "__main__":
  main("dblp.xml")

