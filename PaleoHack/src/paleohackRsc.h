//********************************************************************
// paleohack - Hack 1.0.3 for PalmOS.                                *
// This file is copyright (C) 2001 Bridget Spitznagel                *
//********************************************************************/

#define MainForm              1000
#define ItsyFont              1009
#define MainFormMenu          1010
#define menu_mainAbout        1011
#define menu_mainMap          1012
#define menu_mainQuit         1013
#define menu_mainTest         1014
#define menu_mainWiz1         1015
#define menu_mainRedraw       1016
#define menu_mainMsgs         1019
#define menu_mainFont         1020
#define menu_mainHelp         1021
#define menu_mainScores       1022
#define menu_mainPrefs        1023

#define AboutForm             1050
#define btn_about_ok          1051
#define bitmap_ph             1052
#define bitmap_cat            1053
#define btn_about_license     1054
#define btn_about_credits     1055
#define LicenseStr            1056
#define CreditStr             1057

#define SnowCrashForm         1060
#define SenseForm             1063
#define MapForm               1065
#define TombstoneForm         1067

#define Chargen1Form          1070
#define btn_cg1_no            1071
#define btn_cg1_yes           1072
#define btn_cg1_ok            1073
#define field_charname        1074
#define pbtn_gender_0         1075
#define pbtn_gender_1         1076

#define Chargen2Form          1080
#define btn_cg2_1             1081
#define btn_cg2_2             1082
#define btn_cg2_3             1083
#define btn_cg2_4             1084
#define btn_cg2_5             1085
#define btn_cg2_6             1086

#define InvForm               1100
#define list_if               1101
#define btn_if_frob           1102
#define btn_if_drop           1103
#define btn_if_throw          1104
#define btn_if_cancel         1105
#define btn_if_extra          1106
#define btn_if_dip            1107
#define btn_if_name           1108
#define btn_if_wield          1109
#define btn_if_call           1110
#define InvFormMenu           1130
#define menu_invDip           1131
#define menu_invName          1132
#define menu_invCall          1133
#define menu_invWield         1134
#define menu_invEngrave       1135
#define menu_invMsgs          1139

#define InvActionForm         1150
#define list_iaf              1151
#define btn_ia_frob           1152
#define btn_ia_none           1153
#define btn_ia_all            1154
#define btn_ia_cancel         1155

// a window height:
#define THINGFORM_H 44
#define InvMsgForm            1160



#define MsgLogForm         1310
#define field_ml           1311
#define repeat_ml_up       1312
#define repeat_ml_down     1313
#define btn_ml_ok          1314

#define EngraveForm        1330
#define field_sb           1331
#define btn_sb_ok          1332
#define btn_sb_cancel      1333

#define PrefsForm          1350
#define btn_prefs_ok       1351
#define btn_prefs_cancel   1352
#define label_prf_1        1360
#define label_prf_2        1361
#define list_prf_1         1362
#define list_prf_2         1363
#define check_prf_2        1372
#define check_prf_3        1373
#define check_prf_4        1374
#define check_prf_13       1383
#define check_prf_14       1384

#define ObjTypeForm        1400
#define btn_ot_ok          1401
#define btn_ot_cancel      1402
#define pbtn_ot_0          1410
#define pbtn_ot_1          1411
#define pbtn_ot_2          1412
#define pbtn_ot_3          1413
#define pbtn_ot_4          1414
#define pbtn_ot_5          1415
#define pbtn_ot_6          1416
#define pbtn_ot_7          1417
#define pbtn_ot_8          1418
#define pbtn_ot_9          1419
#define pbtn_ot_10         1420
#define pbtn_ot_11         1421
#define pbtn_ot_12         1422
#define pbtn_ot_13         1423
#define pbtn_ot_MAX        pbtn_ot_13


#define PickUpThisP           1900
#define PICKUP_YES  0
#define PICKUP_NO   1
#define PICKUP_ALL  2
#define PICKUP_QUIT 3
#define RightLeftP            1901
#define EatFloorP             1902
#define IceBoxP               1903
#define NameCallP             1904
#define LongShortP            1905
#define MonsterInfoP          1906

#define Help1Str         2300
#define Help2Str         2301



// Do not mess with these numbers.  They are an ascii-related hack.
#define MAGIC_MENU_NUMBER 4000
#define menu_cmd_CtrlT   4020
#define menu_cmd_space   4032
#define menu_cmd_bang    4033
#define menu_cmd_pound   4035
#define menu_cmd_dollar  4036
#define menu_cmd_paren2  4041
#define menu_cmd_comma   4044
#define menu_cmd_minus   4045
#define menu_cmd_dot     4046
#define menu_cmd_slash   4047
#define menu_cmd_colon   4058
#define menu_cmd_lt      4060
#define menu_cmd_eq      4061
#define menu_cmd_gt      4062
//#define menu_cmd_look    4063
#define menu_cmd_C       4067
#define menu_cmd_D       4068
#define menu_cmd_E       4069
#define menu_cmd_F       4070
#define menu_cmd_G       4071
#define menu_cmd_M       4077
#define menu_cmd_P       4080
#define menu_cmd_quit    4081
#define menu_cmd_R       4082
#define menu_cmd_S       4083
#define menu_cmd_T       4084
#define menu_cmd_W       4087
#define menu_cmd_brack1  4091
#define menu_cmd_bkslash 4092
#define menu_cmd_caret   4094
#define menu_cmd_a       4097
#define menu_cmd_c       4099
#define menu_cmd_d       4100
#define menu_cmd_e       4101
#define menu_cmd_f       4102
#define menu_cmd_i       4105
#define menu_cmd_m       4109
#define menu_cmd_o       4111
#define menu_cmd_p       4112
#define menu_cmd_q       4113
#define menu_cmd_r       4114
#define menu_cmd_s       4115
#define menu_cmd_t       4116
#define menu_cmd_w       4119
#define menu_cmd_x       4120
#define menu_cmd_z       4122
//#define menu_cmd_scribe  4123


// these are for monsters that have "more info".
#define MAGIC_STRING_NUMBER 7000
#define Specify_C  7067
#define Specify_D  7068
#define Specify_G  7071
#define Specify_H  7072
#define Specify_L  7076
#define Specify_c  7099
#define Specify_i  7105
#define Specify_t  7116
#define Specify_u  7117
#define Specify_w  7119
#define Specify_x  7120
#define Specify_z  7122
