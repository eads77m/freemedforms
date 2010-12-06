CREATE TABLE IF NOT EXISTS `INTERACTIONS` (
`ID` INTEGER PRIMARY KEY,
`ATC_ID1` integer NOT NULL ,
`ATC_ID2` integer NOT NULL ,
`INTERACTION_KNOWLEDGE_ID` INTEGER
 );

CREATE TABLE IF NOT EXISTS `INTERACTION_KNOWLEDGE` (
`ID` INTEGER PRIMARY KEY,
`TYPE` VARCHAR(10) NOT NULL ,
`RISK_FR` varchar(2000) NOT NULL ,
`MANAGEMENT_FR` varchar(2000),
`RISK_EN` varchar(2000) NOT NULL ,
`MANAGEMENT_EN` varchar(2000),
`XML` varchar(10000)
 );

-- Generic ATC table (more than 5000 ATC codes are known)
-- Codes ID > 100 000 are interacting molecule names without ATC
-- Codes ID > 200 000 are interactions classes denomination
CREATE TABLE IF NOT EXISTS `ATC` (
`ID` INTEGER PRIMARY KEY,
`CODE` VARCHAR(7) NULL ,
`ENGLISH` VARCHAR(127) NULL ,
`FRENCH` VARCHAR(127) NULL ,
`DEUTSCH` VARCHAR(127) NULL
 );

-- Add IAM classes tree (one class can contains multiple INN)
CREATE TABLE IF NOT EXISTS `IAM_TREE` (
`ID_CLASS` INTEGER NOT NULL,
`ID_ATC`   INTEGER NOT NULL
);

CREATE TABLE IF NOT EXISTS `DB_SCHEMA_VERSION` (
  `VERSION` varchar(10),
  `DATE`  date,
  `COMMENT` varchar(500)
);

INSERT INTO `DB_SCHEMA_VERSION` VALUES ("0.0.8","2009-01-01","First version of the database");
INSERT INTO `DB_SCHEMA_VERSION` VALUES ("0.3.0","2010-03-02","First SVN publication");
INSERT INTO `DB_SCHEMA_VERSION` VALUES ("0.4.4","2010-07-02","Adding the ATC table");
INSERT INTO `DB_SCHEMA_VERSION` VALUES ("0.4.4","2010-07-16","Adding the IAM_TREE table : 1 interacting class <-> N ATC");
INSERT INTO `DB_SCHEMA_VERSION` VALUES ("0.4.4","2010-07-22","Adding INTERACTIONS and INTERACTION_KNOWLEDGE tables");
INSERT INTO `DB_SCHEMA_VERSION` VALUES ("0.4.4","2010-07-22","Removing IAM_DENOMINATION and IAM_IMPORT tables");
INSERT INTO `DB_SCHEMA_VERSION` VALUES ("0.4.4","2010-07-22","Adding DB_SCHEMA_VERSION table");
INSERT INTO `DB_SCHEMA_VERSION` VALUES ("0.4.4","2010-07-22","First english translations of INTERACTION_KNOWLEDGE are available");
