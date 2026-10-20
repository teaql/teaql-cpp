-- Existing query data, separate from schema/bootstrap and graph-save qualification.
CREATE TABLE platform_data(id INTEGER PRIMARY KEY, name TEXT NOT NULL, version INTEGER NOT NULL);
CREATE TABLE specialist_data(id INTEGER PRIMARY KEY, name TEXT NOT NULL,
    platform INTEGER NOT NULL, active INTEGER NOT NULL, version INTEGER NOT NULL);
CREATE TABLE school_data(id INTEGER PRIMARY KEY, name TEXT NOT NULL, rank INTEGER NOT NULL,
    advisor INTEGER, platform INTEGER NOT NULL, version INTEGER NOT NULL);
CREATE TABLE academy_data(id INTEGER PRIMARY KEY, name TEXT NOT NULL, rank INTEGER NOT NULL,
    advisor INTEGER, platform INTEGER NOT NULL, version INTEGER NOT NULL, campus_code TEXT NOT NULL);
CREATE TABLE research_academy_data(id INTEGER PRIMARY KEY, name TEXT NOT NULL, rank INTEGER NOT NULL,
    advisor INTEGER, platform INTEGER NOT NULL, version INTEGER NOT NULL,
    campus_code TEXT NOT NULL, budget NUMERIC NOT NULL);
CREATE TABLE enrollment_data(id INTEGER PRIMARY KEY, school INTEGER NOT NULL,
    note TEXT NOT NULL, version INTEGER NOT NULL);
INSERT INTO platform_data VALUES(1,'root',3);
INSERT INTO specialist_data VALUES(2,'advisor',1,1,3),(3,'deleted advisor',1,0,-1);
INSERT INTO school_data VALUES(7,'school',1,2,1,3),(8,'nullable',2,NULL,1,3),
    (9,'missing',3,99,1,3),(10,'deleted',4,3,1,-1);
INSERT INTO academy_data VALUES(7,'academy',7,2,1,4,'north'),(8,'nullable',8,NULL,1,4,'south'),
    (9,'missing',9,99,1,4,'east'),(10,'deleted',10,3,1,-1,'west');
INSERT INTO research_academy_data VALUES(7,'research',17,2,1,5,'lab',12.50),
    (8,'nullable',18,NULL,1,5,'lab',10.50),(9,'missing',19,99,1,5,'lab',8.50),
    (10,'deleted',20,3,1,-1,'lab',7.50);
INSERT INTO enrollment_data VALUES(21,7,'student',3),(22,7,'second',3),
    (23,8,'nullable school',3),(24,10,'deleted root',-1);
