-- An existing database fixture, separate from generated ensure_schema qualification.
-- Deferred references allow the source's reverse one-to-one metadata cycle.
PRAGMA foreign_keys = ON;
CREATE TABLE platform_data(id INTEGER PRIMARY KEY, name TEXT NOT NULL, version INTEGER NOT NULL);
CREATE TABLE person_data(id INTEGER PRIMARY KEY REFERENCES profile_data(person) DEFERRABLE INITIALLY DEFERRED,
    name TEXT NOT NULL, email TEXT NOT NULL, version INTEGER NOT NULL,
    platform INTEGER NOT NULL REFERENCES platform_data(id));
CREATE TABLE profile_data(id INTEGER PRIMARY KEY, note TEXT, version INTEGER NOT NULL,
    person INTEGER NOT NULL UNIQUE REFERENCES person_data(id) DEFERRABLE INITIALLY DEFERRED);
CREATE TABLE company_data(id INTEGER PRIMARY KEY, name TEXT NOT NULL, version INTEGER NOT NULL,
    platform INTEGER NOT NULL REFERENCES platform_data(id));
CREATE TABLE order_data(id INTEGER PRIMARY KEY, code TEXT NOT NULL, amount NUMERIC NOT NULL,
    priority INTEGER NOT NULL, version INTEGER NOT NULL,
    platform INTEGER NOT NULL REFERENCES platform_data(id),
    customer INTEGER NOT NULL REFERENCES person_data(id), seller INTEGER NOT NULL REFERENCES company_data(id));
CREATE TABLE product_data(id INTEGER PRIMARY KEY, sku TEXT NOT NULL, price NUMERIC NOT NULL,
    version INTEGER NOT NULL, platform INTEGER NOT NULL REFERENCES platform_data(id));
CREATE TABLE line_data(id INTEGER PRIMARY KEY, quantity INTEGER NOT NULL, version INTEGER NOT NULL,
    "order" INTEGER NOT NULL REFERENCES order_data(id), product INTEGER NOT NULL REFERENCES product_data(id));
