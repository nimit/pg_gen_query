-- ============================================================
-- Test Case 4: Complex Schema (Initial State)
-- Drops, recreates, builds schema, indexes, sample data
-- Run with: psql -f init_state.sql postgres
-- ============================================================

DROP DATABASE IF EXISTS change_schema_test;
CREATE DATABASE change_schema_test;

\connect change_schema_test

-- ============================================================
-- Tables
-- ============================================================

CREATE TABLE users (
  id SERIAL PRIMARY KEY,
  name TEXT NOT NULL,
  email TEXT UNIQUE,
  created_at TIMESTAMP DEFAULT NOW()
);

CREATE TABLE products (
  id SERIAL PRIMARY KEY,
  name TEXT NOT NULL,
  category TEXT,
  price NUMERIC NOT NULL
);

CREATE TABLE orders (
  id SERIAL PRIMARY KEY,
  user_id INT NOT NULL REFERENCES users(id),
  total NUMERIC NOT NULL,
  created_at TIMESTAMP DEFAULT NOW()
);

CREATE TABLE order_items (
  id SERIAL PRIMARY KEY,
  order_id INT NOT NULL REFERENCES orders(id) ON DELETE CASCADE,
  product_id INT NOT NULL REFERENCES products(id),
  quantity INT NOT NULL,
  unit_price NUMERIC NOT NULL
);

-- ============================================================
-- Indexes
-- ============================================================

CREATE INDEX idx_users_name ON users(name);
CREATE INDEX idx_users_email ON users(email);

CREATE INDEX idx_products_name ON products(name);
CREATE INDEX idx_products_category ON products(category);

CREATE INDEX idx_orders_user_id ON orders(user_id);
CREATE INDEX idx_orders_created_at ON orders(created_at);

CREATE INDEX idx_order_items_order_id ON order_items(order_id);
CREATE INDEX idx_order_items_product_id ON order_items(product_id);

-- ============================================================
-- Create extension
-- ============================================================

CREATE EXTENSION pg_gen_query;

-- ============================================================
-- Initial data
-- ============================================================

INSERT INTO users (name, email) VALUES
('Alice', 'alice@example.com'),
('Bob', 'bob@example.com'),
('Charlie', 'charlie@example.com');

INSERT INTO products (name, category, price) VALUES
('Laptop', 'Electronics', 1500),
('Phone',  'Electronics', 800),
('Desk',   'Furniture',   300),
('Chair',  'Furniture',   150);

INSERT INTO orders (user_id, total) VALUES
(1, 2300),
(2, 800),
(1, 150);

INSERT INTO order_items (order_id, product_id, quantity, unit_price) VALUES
(1, 1, 1, 1500),  -- Laptop
(1, 2, 1, 800),   -- Phone
(2, 2, 1, 800),   -- Bob buys Phone
(3, 4, 1, 150);   -- Alice buys Chair
