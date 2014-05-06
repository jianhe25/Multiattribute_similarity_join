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
    def __init__(self, out_file):
        xml.sax.ContentHandler.__init__(self)
        self.entityId = 0
        self.articleId = 0
        self.mdate = ""
        self.type = ""
        self.entityKey = ""
        self.title = ""
        self.content = ""
        self.out_file = open(out_file, "w")
        self.out_file.write("entityId \t|title \t|journal \t|author \t|number \t|pages \t|volume \t|year\n");
        self.authors = []
    
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
        self.authors = []

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
        global insertCount
        if name == "article": 
            self.articleId += 1
            authors = ', '.join(self.authors)
            self.out_file.write(str(self.entityId) + "\t|" + self.title + "\t|" + self.journal + "\t|" + 
                authors + "\t|" + self.number + "\t|" + self.pages + "\t|" + self.volume + "\t|" + self.year + "\n")
            insertCount += 1
            if insertCount % 1000 == 0:
                print "Commit {0}".format(insertCount);
                connect.commit();
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
        if name == "author": 
            self.authors.append(content)

    def characters(self, content):
        self.content = self.content + content
 
def main(sourceFileName, out_file):
  source = open(sourceFileName)
  xml.sax.parse(source, ContentHandler(out_file))
 
if __name__ == "__main__":
    if len(sys.argv) != 2:
        print "Usage : python xmlPaser.py out_file"
        sys.exit(2)
    main("./dblp.xml", sys.argv[1])

