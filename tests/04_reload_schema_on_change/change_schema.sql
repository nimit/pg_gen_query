\connect change_schema_test;

-- ============================================================
-- Schema changes to test dynamic regeneration
-- ============================================================

ALTER TABLE orders
  ADD COLUMN status TEXT DEFAULT 'pending';

ALTER TABLE users
  ADD COLUMN loyalty_points INT DEFAULT 0;

CREATE TABLE coupons (
  id SERIAL PRIMARY KEY,
  code TEXT UNIQUE NOT NULL,
  discount NUMERIC NOT NULL
);

CREATE TABLE order_coupons (
  order_id INT NOT NULL REFERENCES orders(id) ON DELETE CASCADE,
  coupon_id INT NOT NULL REFERENCES coupons(id),
  PRIMARY KEY (order_id, coupon_id)
);

-- New indexes
CREATE INDEX idx_orders_status ON orders(status);
CREATE INDEX idx_users_loyalty_points ON users(loyalty_points);
CREATE INDEX idx_coupons_code ON coupons(code);

-- New data
INSERT INTO coupons (code, discount) VALUES
('WELCOME10', 10),
('VIP20', 20);

INSERT INTO order_coupons (order_id, coupon_id) VALUES
(1, 1),
(3, 2);

UPDATE users SET loyalty_points = 100 WHERE id = 1;
