BEGIN;
INSERT INTO platform_data(id,name,version) VALUES(1,'Platform',1);
INSERT INTO person_data(id,platform,name,email,version) VALUES
    (2,1,'Alice','alice@example.test',1),(3,1,'Bob','bob@example.test',2),
    (8,1,'Hidden','hidden@example.test',-1);
INSERT INTO profile_data(id,person,note,version) VALUES(30,2,'alpha',1),(31,3,NULL,1),(32,8,NULL,-1);
INSERT INTO company_data(id,platform,name,version) VALUES(4,1,'Alpha',1),(5,1,'Beta',1);
INSERT INTO product_data(id,platform,sku,price,version) VALUES(6,1,'A',2.5,1),(7,1,'B',3.5,1);
INSERT INTO order_data(id,platform,customer,seller,code,amount,priority,version) VALUES
    (10,1,2,4,'Alpha-10',12,1,1),(11,1,3,5,'Beta-11',8,2,1),
    (12,1,2,5,'Alpha-12',20,3,2),(13,1,2,4,'Hidden',100,4,-1);
INSERT INTO line_data(id,"order",product,quantity,version) VALUES
    (20,10,6,1,1),(21,10,7,3,1),(22,11,7,2,1),(23,12,6,5,-1);
COMMIT;
