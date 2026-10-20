-- Existing rows for canonical relationship query/JSON/save comparisons.
CREATE TABLE platform_data(id INTEGER PRIMARY KEY,version INTEGER NOT NULL,name TEXT NOT NULL);
CREATE TABLE owner_data(id INTEGER PRIMARY KEY,version INTEGER NOT NULL,name TEXT NOT NULL,platform INTEGER NOT NULL);
CREATE TABLE entry_data(id INTEGER PRIMARY KEY,version INTEGER NOT NULL,name TEXT NOT NULL,platform INTEGER NOT NULL,"namespace" INTEGER NOT NULL,"operator" INTEGER);
CREATE TABLE profile_data(id INTEGER PRIMARY KEY,version INTEGER NOT NULL,name TEXT NOT NULL,platform INTEGER NOT NULL,owner INTEGER NOT NULL);
INSERT INTO platform_data VALUES(1,1,'Main');
INSERT INTO owner_data VALUES(10,1,'Alpha',1),(11,1,'Beta',1),(12,-1,'Deleted',1);
INSERT INTO entry_data VALUES(20,1,'First',1,10,11),(21,1,'Optional',1,10,NULL),(22,-1,'Deleted',1,11,10),(23,1,'Other',1,11,10),(24,1,'Hidden owner',1,12,NULL);
INSERT INTO profile_data VALUES(30,1,'Alpha profile',1,10),(31,1,'Beta profile',1,11);
