-- Create DataBase
create database db_arduino;

-- Create Tables
use db_arduino;
CREATE TABLE tb_sensors (
    ID_Time				INT PRIMARY KEY AUTO_INCREMENT,
    Air_Temperature		DECIMAL(5,2),
    Soil_Humidity		DECIMAL(5,2),
    Light_Intensity		DECIMAL(5,2),
    Water_Level			DECIMAL(5,2)
);

CREATE TABLE tb_config (
	ID				INT PRIMARY KEY AUTO_INCREMENT,
    Initial_Time	INT,
    End_Time		INT DEFAULT -1,
    ID_Plant		INT,
    Connection_Time	DATETIME
);	

CREATE VIEW v_thresholders AS
SELECT 
    db_admin.tb_plant_growth.*
FROM 
    db_admin.tb_plant_growth
JOIN 
    db_arduino.tb_config
ON 
    db_admin.tb_plant_growth.ID_Plant = db_arduino.tb_config.ID_Plant
WHERE 
    db_arduino.tb_config.ID = (SELECT MAX(ID) FROM db_arduino.tb_config);