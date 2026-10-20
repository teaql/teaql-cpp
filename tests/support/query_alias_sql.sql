-- Existing rows for default result-alias verification, independent of bootstrap.
CREATE TABLE platform_data(id INTEGER PRIMARY KEY,version INTEGER NOT NULL,name TEXT NOT NULL);
INSERT INTO platform_data VALUES(1,1,'Main');
CREATE TABLE record_data(id INTEGER PRIMARY KEY,version INTEGER NOT NULL,platform INTEGER NOT NULL,
    "class" NUMERIC,"operator" NUMERIC,"template" NUMERIC,"type" INTEGER,"match" NUMERIC,
    ordinary NUMERIC,"namespace" INTEGER);
INSERT INTO record_data VALUES(10,1,1,2,3,4,5,6,7,1),(11,1,1,3,4,5,6,7,8,NULL),
    (12,-1,1,100,100,100,100,100,100,1);
