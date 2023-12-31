-- phpMyAdmin SQL Dump
-- version 5.2.1
-- https://www.phpmyadmin.net/
--
-- Host: localhost
-- Generation Time: Oct 05, 2023 at 11:17 AM
-- Server version: 10.3.32-MariaDB
-- PHP Version: 8.0.23

SET SQL_MODE = "NO_AUTO_VALUE_ON_ZERO";
START TRANSACTION;
SET time_zone = "+00:00";

--
-- Database: `dbPiholeLog`
--
CREATE DATABASE IF NOT EXISTS `dbPiholeLog` DEFAULT CHARACTER SET utf8 COLLATE utf8_general_ci;
USE `dbPiholeLog`;

-- --------------------------------------------------------

--
-- Table structure for table `tblClient`
--

CREATE TABLE `tblClient` (
  `MAC` varchar(17) COLLATE utf8_unicode_ci NOT NULL,
  `clientSubNetID` tinyint(4) UNSIGNED DEFAULT NULL,
  `client` varchar(255) COLLATE utf8_unicode_ci NOT NULL,
  `IP` varchar(24) COLLATE utf8_unicode_ci DEFAULT NULL
) ENGINE=InnoDB DEFAULT CHARSET=utf8 COLLATE=utf8_unicode_ci;

-- --------------------------------------------------------

--
-- Table structure for table `tblCommon`
--

CREATE TABLE `tblCommon` (
  `domainID` bigint(20) NOT NULL
) ENGINE=Aria DEFAULT CHARSET=utf8 PACK_KEYS=0;

-- --------------------------------------------------------

--
-- Table structure for table `tblDomain`
--

CREATE TABLE `tblDomain` (
  `ID` bigint(20) UNSIGNED NOT NULL,
  `domain` varchar(256) NOT NULL,
  `firstSearchDate` datetime NOT NULL,
  `levelOfInterest` tinyint(4) DEFAULT 0,
  `typeID` int(11) DEFAULT 0
) ENGINE=Aria DEFAULT CHARSET=utf8 PACK_KEYS=0;

-- --------------------------------------------------------

--
-- Table structure for table `tblDomainType`
--

CREATE TABLE `tblDomainType` (
  `type` varchar(255) NOT NULL,
  `loiId` int(11) NOT NULL DEFAULT 0,
  `ID` int(11) NOT NULL
) ENGINE=InnoDB DEFAULT CHARSET=utf8;

--
-- Dumping data for table `tblDomainType`
--

INSERT INTO `tblDomainType` (`type`, `loiId`, `ID`) VALUES
('Health', 10, -190),
('Messaging', 20, -180),
('Online applications', 10, -170),
('Industry Orgs', 10, -160),
('Advocacy Groups', 10, -150),
('Content Providers', 10, -140),
('Government', 10, -130),
('Banks and Financial Institutions', 10, -120),
('Analytics Collectors', 10, -110),
('ECommerce', 10, -100),
('Platform Providers', 10, -90),
('Security Providers', 10, -80),
('Advertisers Marketers', 10, -70),
('CDNs', 10, -60),
('Service Providers', 10, -50),
('Media Organisations', 10, -40),
('Retail', 10, -30),
('IST Vendors', 10, -20),
('Education', 10, -10),
('unk', 0, 0),
('Social Media', 20, 10),
('Porn', 20, 20),
('Gaming', 20, 30),
('No Information', 20, 90),
('Potentially Harmful', 20, 100);

-- --------------------------------------------------------

--
-- Table structure for table `tblKnown`
--

CREATE TABLE `tblKnown` (
  `domain` varchar(255) NOT NULL,
  `levelOfInterest` tinyint(4) NOT NULL,
  `comment` varchar(255) DEFAULT NULL COMMENT 'information only',
  `type` int(11) DEFAULT 0
) ENGINE=InnoDB DEFAULT CHARSET=utf8;

-- --------------------------------------------------------

--
-- Table structure for table `tblLevelOfInterest`
--

CREATE TABLE `tblLevelOfInterest` (
  `ID` tinyint(4) NOT NULL,
  `interest` varchar(16) NOT NULL
) ENGINE=InnoDB DEFAULT CHARSET=utf8;

-- --------------------------------------------------------

--
-- Table structure for table `tblLog`
--

CREATE TABLE `tblLog` (
  `domainID` int(11) NOT NULL,
  `clientSubNetID` tinyint(16) UNSIGNED NOT NULL,
  `status` tinyint(4) UNSIGNED NOT NULL,
  `latestSearchDate` datetime NOT NULL,
  `queryCount` int(11) NOT NULL DEFAULT 1
) ENGINE=Aria DEFAULT CHARSET=utf8 PACK_KEYS=0;

-- --------------------------------------------------------

--
-- Table structure for table `tblReadLog`
--

CREATE TABLE `tblReadLog` (
  `readDateTime` datetime NOT NULL DEFAULT current_timestamp(),
  `fileDateTime` datetime NOT NULL,
  `fileSize` int(11) NOT NULL,
  `logEntriesProcessed` int(11) NOT NULL DEFAULT 0,
  `comment` varchar(256) DEFAULT NULL
) ENGINE=Aria DEFAULT CHARSET=utf8 PACK_KEYS=0;

-- --------------------------------------------------------

--
-- Table structure for table `tblStatus`
--

CREATE TABLE `tblStatus` (
  `ID` tinyint(4) NOT NULL,
  `statusTxt` varchar(16) NOT NULL
) ENGINE=InnoDB DEFAULT CHARSET=utf8;

--
-- Dumping data for table `tblStatus`
--

INSERT INTO `tblStatus` (`ID`, `statusTxt`) VALUES
(0, 'forwarded'),
(1, 'blocked'),
(2, 'cached'),
(3, 'blacklisted'),
(127, 'Unknown');

--
-- Indexes for dumped tables
--

--
-- Indexes for table `tblClient`
--
ALTER TABLE `tblClient`
  ADD UNIQUE KEY `MAC` (`MAC`);

--
-- Indexes for table `tblCommon`
--
ALTER TABLE `tblCommon`
  ADD PRIMARY KEY (`domainID`);

--
-- Indexes for table `tblDomain`
--
ALTER TABLE `tblDomain`
  ADD PRIMARY KEY (`ID`),
  ADD UNIQUE KEY `domain` (`domain`);

--
-- Indexes for table `tblDomainType`
--
ALTER TABLE `tblDomainType`
  ADD PRIMARY KEY (`ID`);

--
-- Indexes for table `tblKnown`
--
ALTER TABLE `tblKnown`
  ADD PRIMARY KEY (`domain`);

--
-- Indexes for table `tblLevelOfInterest`
--
ALTER TABLE `tblLevelOfInterest`
  ADD PRIMARY KEY (`ID`);

--
-- Indexes for table `tblLog`
--
ALTER TABLE `tblLog`
  ADD PRIMARY KEY (`domainID`,`clientSubNetID`,`status`);

--
-- Indexes for table `tblReadLog`
--
ALTER TABLE `tblReadLog`
  ADD PRIMARY KEY (`fileDateTime`,`fileSize`);

--
-- AUTO_INCREMENT for dumped tables
--

--
-- AUTO_INCREMENT for table `tblDomain`
--
ALTER TABLE `tblDomain`
  MODIFY `ID` bigint(20) UNSIGNED NOT NULL AUTO_INCREMENT;
COMMIT;
