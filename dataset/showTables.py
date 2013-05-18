import MySQLdb
con = MySQLdb.connect('localhost', 'hejian', 'hejian486', 'dblp')
cur = con.cursor()


def printTable(tableName):
	global cur

	queryStr = """SELECT dblp_entity.*, dblp_entity_article.*, dblp_author.author FROM dblp_entity_article, dblp_entity, dblp_author, dblp_entity_author
			    WHERE dblp_entity_article.entityId = dblp_entity.entityId 
				AND dblp_entity_author.authorId = dblp_entity_article.entityId
				AND dblp_author.authorId 
				AND dblp_entity_author.authorId"""

	print queryStr
	cur.execute(queryStr)

	desc = cur.description
	print desc
	count = 0
	while (count < 1000):
		row = cur.fetchone()
		print row
		count += 1

tableSet = set(["article", "inproceedings", "proceedings", "book",
				"incollection", "phdthesis", "msthesis", "www"])

printTable("article")

