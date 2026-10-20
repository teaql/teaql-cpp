-- Existing test data; generated schema/bootstrap qualification remains separate.
CREATE TABLE tenant_data(id INTEGER PRIMARY KEY, version INTEGER NOT NULL, name TEXT NOT NULL);
CREATE TABLE entry_data(
    id INTEGER PRIMARY KEY, version INTEGER NOT NULL, tenant INTEGER NOT NULL,
    label TEXT NOT NULL, optional_label TEXT, notes TEXT NOT NULL, secret TEXT NOT NULL,
    active INTEGER NOT NULL, optional_active INTEGER, amount NUMERIC NOT NULL,
    optional_amount NUMERIC, height REAL NOT NULL, ratio REAL NOT NULL,
    counter INTEGER NOT NULL, day TEXT NOT NULL, optional_day TEXT,
    moment TEXT NOT NULL, optional_moment TEXT);
INSERT INTO tenant_data VALUES(1,1,'Main'),(7,1,'Other');
INSERT INTO entry_data VALUES
    (10,1,1,'First',NULL,'Notes','secret',1,NULL,2.5,NULL,2.5,2.5,3,
        '2026-10-14',NULL,'2026-10-14T00:00:00Z',NULL),
    (11,2,1,'Second','Optional','Notes','secret',0,1,3.5,4,3.5,3.5,4,
        '2026-10-14','2026-10-15','2026-10-14T00:00:00Z','2026-10-15T00:00:00Z'),
    (12,1,7,'Other',NULL,'Notes','secret',1,NULL,7,NULL,7,7,7,
        '2026-10-14',NULL,'2026-10-14T00:00:00Z',NULL),
    (13,-1,1,'Deleted',NULL,'Notes','secret',1,NULL,100,NULL,100,100,100,
        '2026-10-14',NULL,'2026-10-14T00:00:00Z',NULL);
