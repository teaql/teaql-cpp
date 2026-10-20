-- Existing rows, independent of schema/bootstrap and identifier allocation.
CREATE TABLE tenant_data(id INTEGER PRIMARY KEY,name TEXT NOT NULL,version INTEGER NOT NULL);
CREATE TABLE receipt_data(id INTEGER PRIMARY KEY,name TEXT NOT NULL,tenant INTEGER NOT NULL,
    receipt_number TEXT NOT NULL,version INTEGER NOT NULL);
CREATE TABLE ticket_data(id INTEGER PRIMARY KEY,name TEXT NOT NULL,tenant INTEGER NOT NULL,
    ticket_number TEXT NOT NULL,version INTEGER NOT NULL);
INSERT INTO tenant_data VALUES(1,'first tenant',1),(2,'second tenant',1);
INSERT INTO receipt_data VALUES(7,'first',1,'match',1),(8,'second',1,'match',1),
    (9,'third',1,'match',1),(10,'deleted',1,'match',-1),(11,'other tenant',2,'match',1);
INSERT INTO ticket_data VALUES(7,'first',1,'match',1),(8,'second',1,'match',1),
    (9,'third',1,'match',1),(10,'deleted',1,'match',-1),(11,'other tenant',2,'match',1);
