-- ============================================================
-- Reset the "test" database
-- Drops if exists, recreates, and fills with dummy data
-- Run with: psql -f reset_test_db.sql postgres
-- ============================================================

-- Drop and recreate test database
DROP DATABASE IF EXISTS test;
CREATE DATABASE test;

\connect test

-- ============================================================
-- Schema setup
-- ============================================================

CREATE SCHEMA IF NOT EXISTS demo;

-- ============================================================
-- Tables
-- ============================================================

-- Users table
CREATE TABLE demo.users (
    id SERIAL PRIMARY KEY,
    username TEXT NOT NULL,
    email TEXT NOT NULL UNIQUE,
    signup_date TIMESTAMP DEFAULT NOW()
);

-- Products table
CREATE TABLE demo.products (
    id SERIAL PRIMARY KEY,
    name TEXT NOT NULL,
    category TEXT,
    price NUMERIC(10,2),
    stock INT DEFAULT 0
);

-- Orders table
CREATE TABLE demo.orders (
    id SERIAL PRIMARY KEY,
    user_id INT REFERENCES demo.users(id),
    product_id INT REFERENCES demo.products(id),
    quantity INT NOT NULL,
    order_date TIMESTAMP DEFAULT NOW()
);

-- ============================================================
-- Dummy data
-- ============================================================

INSERT INTO demo.users (username, email)
VALUES
('alice', 'alice@example.com'),
('bob', 'bob@example.com'),
('charlie', 'charlie@example.com'),
('diana', 'diana@example.com'),
('eve', 'eve@example.com');

INSERT INTO demo.products (name, category, price, stock)
VALUES
('Laptop', 'Electronics', 1200.00, 15),
('Phone', 'Electronics', 799.99, 30),
('Book', 'Education', 19.99, 100),
('Headphones', 'Electronics', 59.99, 50),
('Coffee Mug', 'Home', 9.99, 200);

INSERT INTO demo.orders (user_id, product_id, quantity, order_date)
VALUES
(1, 1, 1, NOW() - INTERVAL '10 days'),
(2, 3, 2, NOW() - INTERVAL '5 days'),
(3, 2, 1, NOW() - INTERVAL '3 days'),
(1, 5, 4, NOW() - INTERVAL '1 day'),
(4, 4, 2, NOW() - INTERVAL '2 hours');

-- ============================================================
-- Useful indexes for query-based extensions
-- ============================================================

CREATE INDEX idx_orders_user_id ON demo.orders(user_id);
CREATE INDEX idx_orders_product_id ON demo.orders(product_id);
CREATE INDEX idx_products_category ON demo.products(category);

-- ============================================================
-- Optional: grant privileges to current user
-- ============================================================

GRANT ALL PRIVILEGES ON SCHEMA demo TO PUBLIC;
GRANT ALL PRIVILEGES ON ALL TABLES IN SCHEMA demo TO PUBLIC;
GRANT ALL PRIVILEGES ON ALL SEQUENCES IN SCHEMA demo TO PUBLIC;

-- ============================================================
-- Ready for use!
-- ============================================================

\echo '✅ Test database test created and populated.'
