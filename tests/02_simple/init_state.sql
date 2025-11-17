-- ============================================================
-- Reset the "simple_test" database
-- Drops if exists, recreates, and fills with dummy data
-- Run with: psql -f init_state.sql postgres
-- ============================================================

-- =========================================
-- Drop and recreate database
-- =========================================

DROP DATABASE IF EXISTS simple_test;
CREATE DATABASE simple_test;

\connect simple_test

-- =========================================
-- Simple schema
-- =========================================

CREATE TABLE customers (
  id SERIAL PRIMARY KEY,
  name TEXT,
  age INT
);

CREATE TABLE orders (
  id SERIAL PRIMARY KEY,
  customer_id INT REFERENCES customers,
  item TEXT,
  price NUMERIC
);

-- =========================================
-- Indexes
-- =========================================

CREATE INDEX idx_customers_name ON customers(name);
CREATE INDEX idx_customers_age  ON customers(age);

CREATE INDEX idx_orders_customer_id ON orders(customer_id);
CREATE INDEX idx_orders_item        ON orders(item);
CREATE INDEX idx_orders_price       ON orders(price);

CREATE EXTENSION pg_gen_query;

-- =========================================
-- Insert sample data (small test set)
-- =========================================

INSERT INTO customers (name, age) VALUES
('Alice', 30),
('Bob',   40),
('Charlie', 25),
('David', 50);

INSERT INTO orders (customer_id, item, price) VALUES
(1, 'Laptop',    1200.00),
(1, 'Phone',      800.00),
(2, 'Tablet',     400.00),
(3, 'Headphones', 150.00),
(4, 'Laptop',    1400.00);
