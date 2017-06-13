create table if not exists folders (path VARCHAR NOT NULL);
create table if not exists files (set_id integer default 0, path VARCHAR NOT NULL, enabled integer default 1, position integer);
create table if not exists folder_sets (id integer, name varchar);
INSERT INTO sqlite_sequence(name, seq) SELECT 'folder_sets', 0 WHERE NOT EXISTS(SELECT 1 FROM sqlite_sequence WHERE name = 'folder_sets');
INSERT INTO folder_sets(name) SELECT 'base' WHERE NOT EXISTS(SELECT 1 FROM folder_sets WHERE name = 'base');