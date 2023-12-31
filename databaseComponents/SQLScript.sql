DELIMITER $$
CREATE DEFINER=`root`@`localhost` PROCEDURE `addLogEntry`(IN `inDomain` VARCHAR(256), IN `inClientID` TINYINT UNSIGNED, IN `inStatus` TINYINT UNSIGNED, IN `inLogDate` DATETIME)
    MODIFIES SQL DATA
    SQL SECURITY INVOKER
    COMMENT 'Insert a new log entry; create the domain entry if neccessary'
BEGIN
	DECLARE existingDomainID INT DEFAULT 0;
    
    -- If the domain does not exist (i.e. existingDomainId == 0) INSERT a new domain and Log Entry as an EXIT Handler for the NOT FOUND condition
    -- else just insert a new log entry using the existing domain id
	DECLARE EXIT HANDLER FOR NOT FOUND
	BEGIN
		INSERT INTO tblDomain (ID, domain, firstSearchDate) VALUES (NULL, inDomain, inLogDate); 
        INSERT INTO tblLog (domainID, clientSubNetID, status, latestSearchDate, queryCount) VALUES (LAST_INSERT_ID(), inClientID, inStatus, inLogDate, 1); 
	END;
   
	-- See if the domain has already been stored (triggers the NOT FOUND EXIT Handler)
	SELECT tblDomain.ID INTO existingDomainID FROM tblDomain WHERE domain = inDomain;
	-- If the domain was found then existingDomainID will be non-zero after this call

	-- See if the client has had another call to that domain with that status
    IF 0=(SELECT COUNT(*) FROM tblLog WHERE domainID = existingDomainID AND status = inStatus AND clientSubNetID = inClientID) THEN
   		INSERT INTO tblLog (domainID, clientSubNetID, status, latestSearchDate, queryCount) VALUES (existingDomainID, inClientID, inStatus, inLogDate, 1); -- , inLOI);
    ELSE
    	UPDATE tblLog SET latestSearchDate = GREATEST(inLogDate, tblLog.latestSearchDate), queryCount = queryCount + 1 WHERE domainID = existingDomainID AND status = inStatus AND clientSubNetID = inClientID; 
    END IF;
    
END$$
DELIMITER ;

DELIMITER $$
CREATE DEFINER=`root`@`localhost` PROCEDURE `appendToCommon`()
    MODIFIES SQL DATA
    SQL SECURITY INVOKER
    COMMENT 'Appends common domains in tblLog to tblCommon'
BEGIN
    INSERT IGNORE INTO tblCommon SELECT `tblDomain`.`ID` from tblDomain, tblLog WHERE tblLog.domainID = tblDomain.ID GROUP BY tblLog.domainID HAVING  SUM(`tblLog`.`queryCount`) > 20000;
    UPDATE tblDomain SET levelOfInterest = 10 WHERE ID IN (SELECT domainID FROM tblCommon WHERE 1);
END$$
DELIMITER ;

DELIMITER $$
CREATE DEFINER=`root`@`localhost` PROCEDURE `lookupSiteOverDays`(IN `site` VARCHAR(255), IN `days` INT)
    SQL SECURITY INVOKER
BEGIN
	DECLARE oneDayAgo DATETIME;
	SELECT max(`tblLog`.`latestSearchDate`) INTO oneDayAgo FROM `tblLog`; 
	SELECT `dbPiholeLog`.`tblDomain`.`domain` AS `Domain`, `dbPiholeLog`.`tblDomain`.`levelOfInterest` AS `LOI`,`dbPiholeLog`.`tblLog`.`clientSubNetID` AS `Client`,`dbPiholeLog`.`tblLog`.`latestSearchDate` AS `SearchDate`,`dbPiholeLog`.`tblLog`.`queryCount` AS `Count`, tblStatus.statusTxt AS `Status` from `dbPiholeLog`.`tblDomain`, `dbPiholeLog`.`tblLog`, tblStatus where `dbPiholeLog`.`tblDomain`.`ID` = `dbPiholeLog`.`tblLog`.`domainID` and tblLog.status = tblStatus.ID and `dbPiholeLog`.`tblLog`.`clientSubNetID` <> 102 and `dbPiholeLog`.`tblLog`.`latestSearchDate` > oneDayAgo - INTERVAL days  Day and tblDomain.domain LIKE CONCAT('%',site,'%') order by `dbPiholeLog`.`tblLog`.`latestSearchDate`;
END$$
DELIMITER ;

DELIMITER $$
CREATE DEFINER=`logUser`@`%` PROCEDURE `maxDate`()
    SQL SECURITY INVOKER
    COMMENT 'Return the date-time of the most recently stored domain'
BEGIN
    SELECT max(`tblLog`.`latestSearchDate`)  FROM `tblLog`;
END$$
DELIMITER ;

DELIMITER $$
CREATE DEFINER=`logUser`@`192.168.1.144` PROCEDURE `maxDateYYYYMMDDHourMinSec`()
    READS SQL DATA
    SQL SECURITY INVOKER
BEGIN
	DECLARE maxD DATETIME;
    SELECT max(`latestSearchDate`) INTO maxD FROM tblLog;
	SELECT YEAR(maxD) AS YYYY, MONTH(maxD) as MM, DAY(maxD) AS DD, HOUR(maxD) as Hour, MINUTE(maxD) as Min, SECOND(maxD) as Sec;
END$$
DELIMITER ;

DELIMITER $$
CREATE DEFINER=`root`@`localhost` PROCEDURE `mostRecent`()
    READS SQL DATA
    SQL SECURITY INVOKER
    COMMENT 'Show a table of all calls in the last day'
BEGIN
	DECLARE oneDayAgo DATETIME;
	SELECT max(`tblLog`.`latestSearchDate`) INTO oneDayAgo FROM `tblLog`; 
	SELECT `dbPiholeLog`.`tblDomain`.`domain` AS `Domain`, 
    		`dbPiholeLog`.`tblDomain`.`levelOfInterest` AS `LOI`, 
            `dbPiholeLog`.`tblLog`.`clientSubNetID` AS `Client`, 
            `dbPiholeLog`.`tblLog`.`latestSearchDate` AS `SearchDate`, 
            `dbPiholeLog`.`tblLog`.`queryCount` AS `Count`, 
            `dbPiholeLog`.`tblStatus`.`statusTxt` AS `StatusTxt`, 
            `dbPiholeLog`.`tblDomainType`.`type` AS `Type` 
            FROM `dbPiholeLog`.`tblDomain`, 
            	`dbPiholeLog`.`tblLog`, 
                `dbPiholeLog`.`tblStatus`,
                `dbPiholeLog`.`tblDomainType`
            WHERE `dbPiholeLog`.`tblDomain`.`ID` = `dbPiholeLog`.`tblLog`.`domainID` 
            	and `dbPiholeLog`.`tblDomainType`.`ID` = `dbPiholeLog`.`tblDomain`.`typeID`
            	and `dbPiholeLog`.`tblLog`.`status` = `dbPiholeLog`.`tblStatus`.`ID` 
                and `dbPiholeLog`.`tblDomain`.`levelOfInterest` <> 10 
                and `dbPiholeLog`.`tblLog`.`latestSearchDate` > oneDayAgo - INTERVAL 1 Day 
                and tblDomain.levelOfInterest <> 10 order by `dbPiholeLog`.`tblLog`.`latestSearchDate`;
END$$
DELIMITER ;

DELIMITER $$
CREATE DEFINER=`root`@`localhost` PROCEDURE `recordLogFile`(IN `inFileSize` INT UNSIGNED, IN `inFileDateTime` DATETIME)
    MODIFIES SQL DATA
    SQL SECURITY INVOKER
BEGIN 
	INSERT INTO tblReadLog (fileSize, fileDateTime) VALUES (inFileSize, inFileDateTime);
END$$
DELIMITER ;

DELIMITER $$
CREATE DEFINER=`root`@`localhost` PROCEDURE `refreshAllTypeLOI`()
    NO SQL
    SQL SECURITY INVOKER
CALL updateAllLOIFromDate('1970-01-01 00:00:00')$$
DELIMITER ;

DELIMITER $$
CREATE DEFINER=`root`@`localhost` PROCEDURE `updateAllLOIFromDate`(IN `inUpdateFromDate` DATE)
    COMMENT 'Iterate through tblKnow updating the levelOfInterest'
BEGIN
	DECLARE finished INTEGER DEFAULT 0;
    DECLARE rx CHAR(255);
    DECLARE loi TINYINT;
    DECLARE type INT;
    
    DECLARE cur CURSOR FOR SELECT tblKnown.domain, tblKnown.levelOfInterest, tblKnown.type FROM tblKnown WHERE 1;
    DECLARE CONTINUE HANDLER FOR NOT FOUND SET finished = 1;
    
    OPEN cur;
    rxLoop: LOOP
    	FETCH cur INTO rx, loi, type;
        IF finished = 1 THEN
        	LEAVE rxLoop;
        END IF;
        
		UPDATE tblDomain 
        SET tblDomain.levelOfInterest = loi, tblDomain.typeID = type 
        WHERE tblDomain.domain RLIKE rx AND tblDomain.firstSearchDate > inUpdateFromDate AND (tblDomain.levelOfInterest <> loi OR tblDomain.typeID <> type);
        
        -- SELECT tblDomain.* FROM tblDomain WHERE tblDomain.domain RLIKE rx AND tblDomain.firstSearchDate > inUpdateFromDate AND (tblDomain.levelOfInterest <> loi OR tblDomain.typeID <> type);
        -- SELECT rx, loi, type;
        
    END LOOP rxLoop;
    CLOSE cur;
END$$
DELIMITER ;

DELIMITER $$
CREATE DEFINER=`root`@`localhost` PROCEDURE `updateLogFileRecord`(IN `inFileSize` INT UNSIGNED, IN `inFileDateTime` DATETIME, IN `inLogEntriesProcessed` INT UNSIGNED, IN `inComment` VARCHAR(256))
    MODIFIES SQL DATA
    SQL SECURITY INVOKER
BEGIN
	UPDATE tblReadLog SET logEntriesProcessed = inLogEntriesProcessed, comment = inComment WHERE fileSize = inFileSize AND fileDateTime = inFileDateTime;
END$$
DELIMITER ;
