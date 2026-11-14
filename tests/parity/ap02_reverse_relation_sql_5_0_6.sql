CREATE TABLE platform_data (id INTEGER PRIMARY KEY, name TEXT, version INTEGER);
CREATE TABLE business_items (
    id INTEGER PRIMARY KEY, display_name TEXT, note TEXT, version INTEGER, platform INTEGER
);
CREATE TABLE business_children (
    id INTEGER PRIMARY KEY, display_name TEXT, note TEXT, version INTEGER,
    platform INTEGER, password TEXT, owner INTEGER
);
INSERT INTO platform_data VALUES (1, 'platform', 1);
INSERT INTO business_items VALUES
    (10, 'parent', NULL, 1, 1),
    (11, 'empty', NULL, 1, 1),
    (20, 'first child', NULL, 1, 1),
    (21, 'second child', NULL, 1, 1);
INSERT INTO business_children VALUES
    (20, 'first child', NULL, 1, 1, 'alpha', 10),
    (21, 'second child', NULL, 1, 1, 'beta', 10);
