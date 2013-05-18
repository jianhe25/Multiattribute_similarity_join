import MySQLdb as mdb
import sys

def dropTables(cur):
	cur.execute("""DROP TABLE dblp_entity""")
	cur.execute("""DROP TABLE dblp_entity_article""")
	cur.execute("""DROP TABLE dblp_entity_book""")
	cur.execute("""DROP TABLE dblp_entity_incollection""")
	cur.execute("""DROP TABLE dblp_entity_inproceedings""")
	cur.execute("""DROP TABLE dblp_entity_proceedings""")
	cur.execute("""DROP TABLE dblp_entity_msthesis""")
	cur.execute("""DROP TABLE dblp_entity_phdthesis""")
	cur.execute("""DROP TABLE dblp_author""")
	cur.execute("""DROP TABLE dblp_entity_author""")
	cur.execute("""DROP TABLE dblp_editor""")
	cur.execute("""DROP TABLE dblp_entity_editor""")
	cur.execute("""DROP TABLE dblp_entity_www""")
	cur.execute("""DROP TABLE dblp_entity_url""")

con = mdb.connect('localhost', 'hejian', 'hejian486', 'dblp')
with con:
	cur = con.cursor()
#######################################################################
# Drop all tables, VERY DANGEROUS!!!!!!!!!!!!!!
	dropTables(cur)
######################################################################

	cur.execute("""CREATE TABLE IF NOT EXISTS dblp_entity(
    	entityId INT PRIMARY KEY, 
		entityKey VARCHAR(100), 
		mdate VARCHAR(100),
		type VARCHAR(100), 
		title VARCHAR(500))""")

	cur.execute("""CREATE TABLE IF NOT EXISTS dblp_entity_article(
    	entityId INT PRIMARY KEY REFERENCES dblp_entity(entityId), 
		journal VARCHAR(100), 
		number VARCHAR(100),
		pages VARCHAR(100), 
		volume VARCHAR(100),
		year VARCHAR(100))""")

	cur.execute("""CREATE TABLE IF NOT EXISTS dblp_entity_book(
    	entityId INT PRIMARY KEY REFERENCES dblp_entity(entityId), 
		booktitle VARCHAR(500), 
		series VARCHAR(100),
		volume VARCHAR(100), 
		year VARCHAR(100))""")

	cur.execute("""CREATE TABLE IF NOT EXISTS dblp_entity_proceedings(
    	entityId INT PRIMARY KEY REFERENCES dblp_entity(entityId), 
		booktitle VARCHAR(500), 
		series VARCHAR(100),
		volume VARCHAR(100),
		year VARCHAR(100))""")

	cur.execute("""CREATE TABLE IF NOT EXISTS dblp_entity_incollection(
    	entityId INT PRIMARY KEY REFERENCES dblp_entity(entityId), 
		booktitle VARCHAR(500), 
		crossref VARCHAR(100) REFERENCES dblp_entity(entityKey),
		pages VARCHAR(100), 
		year VARCHAR(100))""")

	cur.execute("""CREATE TABLE IF NOT EXISTS dblp_entity_inproceedings(
    	entityId INT PRIMARY KEY REFERENCES dblp_entity(entityId), 
		booktitle VARCHAR(500), 
		crossref VARCHAR(100) REFERENCES dblp_entity(entityKey),
		pages VARCHAR(100), 
		year VARCHAR(100))""")

	cur.execute("""CREATE TABLE IF NOT EXISTS dblp_entity_www(
    	entityId INT PRIMARY KEY REFERENCES dblp_entity(entityId), 
		url VARCHAR(100), 
		year VARCHAR(100))""")

	cur.execute("""CREATE TABLE IF NOT EXISTS dblp_entity_msthesis(
    	entityId INT PRIMARY KEY REFERENCES dblp_entity(entityId), 
		school VARCHAR(100), 
		year VARCHAR(100))""")

	cur.execute("""CREATE TABLE IF NOT EXISTS dblp_entity_phdthesis(
    	entityId INT PRIMARY KEY REFERENCES dblp_entity(entityId), 
		school VARCHAR(100), 
		year VARCHAR(100))""")


	cur.execute("""CREATE TABLE IF NOT EXISTS dblp_author(
		authorId INT PRIMARY KEY,
		author VARCHAR(100))""")

	cur.execute("""CREATE TABLE IF NOT EXISTS dblp_entity_author(
    	entityId INT REFERENCES dblp_entity(entityId), 
		authorId INT REFERENCES dblp_editor(authorId),
		PRIMARY KEY(entityId, authorId))""")

	cur.execute("""CREATE TABLE IF NOT EXISTS dblp_editor(
		editorId INT PRIMARY KEY,
		editor VARCHAR(100))""")

	cur.execute("""CREATE TABLE IF NOT EXISTS dblp_entity_editor(
    	entityId INT REFERENCES dblp_entity(entityId), 
		editorId INT REFERENCES dblp_editor(editorId),
		PRIMARY KEY(entityId, editorId))""")

	cur.execute("""CREATE TABLE IF NOT EXISTS dblp_entity_url(
		entityId INT PRIMARY KEY REFERENCES dblp_entity(entityId),
		url VARCHAR(100))""")


