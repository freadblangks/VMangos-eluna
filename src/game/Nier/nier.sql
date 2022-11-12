DROP TABLE IF EXISTS `nier`;
CREATE TABLE `nier` (
  `entry` int(11) NOT NULL AUTO_INCREMENT,
  `nier_id` int(11) NOT NULL DEFAULT '0',
  `account_name` varchar(50) COLLATE utf8_unicode_ci NOT NULL,
  `character_id` int(11) NOT NULL DEFAULT '0',
  `target_level` int(11) NOT NULL DEFAULT '0',
  `nier_type` int(11) NOT NULL,
  PRIMARY KEY (`entry`)
) ENGINE=InnoDB AUTO_INCREMENT=851 DEFAULT CHARSET=utf8 COLLATE=utf8_unicode_ci;
