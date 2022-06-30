CREATE TABLE wx (
    time TIMESTAMP DEFAULT NOW(),
    temperature DECIMAL(5,2),
    pressure SMALLINT UNSIGNED,
    lux SMALLINT UNSIGNED,
    humidity DECIMAL(5,2)
);