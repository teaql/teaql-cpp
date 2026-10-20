-- Existing data for query qualification, independent of schema/bootstrap and ID allocation.
CREATE TABLE platform_data(id INTEGER PRIMARY KEY, version INTEGER NOT NULL, name TEXT NOT NULL);
CREATE TABLE order_status_data(id INTEGER PRIMARY KEY, version INTEGER NOT NULL,
    platform INTEGER NOT NULL, name TEXT NOT NULL, code TEXT NOT NULL);
CREATE TABLE record_data(id INTEGER PRIMARY KEY, version INTEGER NOT NULL,
    platform INTEGER NOT NULL, name TEXT NOT NULL, status INTEGER NOT NULL,
    phase TEXT NOT NULL, tag TEXT);
CREATE TABLE person_data(id INTEGER PRIMARY KEY, version INTEGER NOT NULL,
    platform INTEGER NOT NULL, name TEXT NOT NULL, status INTEGER NOT NULL, phase TEXT NOT NULL);
CREATE TABLE tenant_data(id INTEGER PRIMARY KEY, version INTEGER NOT NULL, name TEXT NOT NULL);
CREATE TABLE receipt_data(id INTEGER PRIMARY KEY, version INTEGER NOT NULL,
    tenant INTEGER NOT NULL, name TEXT NOT NULL, receipt_number TEXT NOT NULL);
CREATE TABLE ticket_data(id INTEGER PRIMARY KEY, version INTEGER NOT NULL,
    tenant INTEGER NOT NULL, name TEXT NOT NULL, ticket_number TEXT NOT NULL);
INSERT INTO platform_data VALUES(1,1,'Main'),(2,1,'Other');
INSERT INTO order_status_data VALUES(11,1,1,'Draft order','DRAFT'),
    (22,1,1,'In progress order','IN_PROGRESS'),(33,1,2,'Completed order','DONE');
INSERT INTO record_data VALUES(10,1,1,'First',11,'Draft','red'),
    (20,2,1,'Second',22,'InProgress',NULL),(30,-1,1,'Deleted',33,'done','blue');
INSERT INTO person_data VALUES(10,1,1,'Alice',11,'Draft'),
    (20,2,1,'Bob',22,'InProgress'),(30,-1,1,'Deleted',33,'done');
INSERT INTO tenant_data VALUES(1,1,'Main'),(2,1,'Other');
INSERT INTO receipt_data VALUES(10,1,1,'First','RCP-00001'),
    (20,2,1,'Second','RCP-00002'),(30,-1,2,'Deleted','RCP-00003');
INSERT INTO ticket_data VALUES(10,1,1,'First','TKT-00000001'),
    (20,2,1,'Second','TKT-00000002'),(30,-1,2,'Deleted','TKT-00000003');
