-- Create DataBase
create database db_admin;

-- Create Tables
use db_admin;
CREATE TABLE tb_plants (
    ID_Plant		INT PRIMARY KEY AUTO_INCREMENT,
    Name_plant		VARCHAR(100)
);

CREATE TABLE tb_user_plant (
    ID				INT PRIMARY KEY AUTO_INCREMENT,
    ID_user			INT,
    Arduino			VARCHAR(100),
    ID_Plant		INT,
    FOREIGN KEY (ID_Plant) REFERENCES tb_plants(ID_Plant) ON DELETE CASCADE
);	

CREATE TABLE tb_plant_growth (
	ID					INT PRIMARY KEY AUTO_INCREMENT,
    ID_Plant			INT,
    Growth_Stage		VARCHAR(100),
    Start_Week			INT,
    End_Week			INT,
    Air_Temperature_Min DECIMAL(5,2),
    Air_Temperature_Max DECIMAL(5,2),
    Soil_Humidity_Min 	DECIMAL(5,2),
    Soil_Humidity_Max 	DECIMAL(5,2),
    Light_Intensity_Min DECIMAL(5,2),
    Light_Intensity_Max DECIMAL(5,2),
    Light_Exposure_Min 	DECIMAL(5,2),
    Light_Exposure_Max 	DECIMAL(5,2),
    Fertilization 		VARCHAR(100),
    FOREIGN KEY (ID_Plant) REFERENCES tb_plants(ID_Plant) ON DELETE CASCADE
);
