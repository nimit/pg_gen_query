-- ============================================================
-- Reset the "complex_test" database
-- Drops if exists, recreates, and fills with dummy data
-- Run with: psql -f init_state.sql postgres
-- ============================================================

-- =========================================
-- Drop and recreate database
-- =========================================

DROP DATABASE IF EXISTS complex_test;
CREATE DATABASE complex_test;

\connect complex_test

-- =========================================
-- Complex schema
-- =========================================

CREATE TABLE users (
  user_id SERIAL PRIMARY KEY,
  name TEXT,
  email TEXT
);

CREATE TABLE addresses (
  address_id SERIAL PRIMARY KEY,
  user_id INT REFERENCES users,
  city TEXT,
  country TEXT
);

CREATE TABLE products (
  product_id SERIAL PRIMARY KEY,
  name TEXT,
  category TEXT,
  price NUMERIC
);

CREATE TABLE warehouses (
  warehouse_id SERIAL PRIMARY KEY,
  name TEXT,
  region TEXT
);

CREATE TABLE inventory (
  inventory_id SERIAL PRIMARY KEY,
  product_id INT REFERENCES products,
  warehouse_id INT REFERENCES warehouses,
  quantity INT
);

CREATE TABLE orders (
  order_id SERIAL PRIMARY KEY,
  user_id INT REFERENCES users,
  order_date DATE
);

CREATE TABLE order_items (
  order_id INT REFERENCES orders,
  product_id INT REFERENCES products,
  quantity INT,
  PRIMARY KEY (order_id, product_id)
);

CREATE TABLE shipments (
  shipment_id SERIAL PRIMARY KEY,
  order_id INT REFERENCES orders,
  warehouse_id INT REFERENCES warehouses,
  shipped_date DATE
);

-- =========================================
-- Indexes
-- =========================================

CREATE INDEX idx_users_email ON users(email);
CREATE INDEX idx_users_name ON users(name);

CREATE INDEX idx_addresses_user_id ON addresses(user_id);
CREATE INDEX idx_addresses_city ON addresses(city);
CREATE INDEX idx_addresses_country ON addresses(country);

CREATE INDEX idx_products_category ON products(category);
CREATE INDEX idx_products_price ON products(price);
CREATE INDEX idx_products_name ON products(name);

CREATE INDEX idx_warehouses_region ON warehouses(region);
CREATE INDEX idx_warehouses_name ON warehouses(name);

CREATE INDEX idx_inventory_product_id ON inventory(product_id);
CREATE INDEX idx_inventory_warehouse_id ON inventory(warehouse_id);
CREATE INDEX idx_inventory_quantity ON inventory(quantity);

CREATE INDEX idx_orders_user_id ON orders(user_id);
CREATE INDEX idx_orders_order_date ON orders(order_date);

CREATE INDEX idx_order_items_order_id ON order_items(order_id);
CREATE INDEX idx_order_items_product_id ON order_items(product_id);
CREATE INDEX idx_order_items_order_product ON order_items(order_id, product_id);

CREATE INDEX idx_shipments_order_id ON shipments(order_id);
CREATE INDEX idx_shipments_warehouse_id ON shipments(warehouse_id);
CREATE INDEX idx_shipments_shipped_date ON shipments(shipped_date);

CREATE EXTENSION pg_gen_query;

-- =========================================
-- Insert a small deterministic dataset
-- (Good for correctness tests)
-- =========================================

INSERT INTO users (name, email) VALUES
('Alice',  'alice@example.com'),
('Bob',    'bob@example.com'),
('Carol',  'carol@example.com'),
('David',  'david@example.com');

INSERT INTO addresses (user_id, city, country) VALUES
(1, 'New York', 'USA'),
(2, 'London',   'UK'),
(3, 'Paris',    'France'),
(4, 'Berlin',   'Germany');

INSERT INTO products (name, category, price) VALUES
('Laptop Pro',   'Electronics', 1500),
('Shirt Linen',  'Clothing',     60),
('Tennis Racket','Sports',      120),
('Perfume',      'Beauty',      200);

INSERT INTO warehouses (name, region) VALUES
('WH-East',  'East'),
('WH-West',  'West'),
('WH-North', 'North');

INSERT INTO inventory (product_id, warehouse_id, quantity) VALUES
(1, 1, 10),
(2, 1, 200),
(3, 2, 5),
(4, 3, 50);

INSERT INTO orders (user_id, order_date) VALUES
(1, '2024-01-10'),
(2, '2024-01-11'),
(3, '2024-01-12');

INSERT INTO order_items (order_id, product_id, quantity) VALUES
(1, 1, 1),
(1, 2, 3),
(2, 3, 2),
(3, 4, 1);

INSERT INTO shipments (order_id, warehouse_id, shipped_date) VALUES
(1, 1, '2024-01-11'),
(2, 2, '2024-01-12'),
(3, 3, '2024-01-13');
