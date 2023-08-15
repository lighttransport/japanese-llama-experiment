// generated from ../models/tinysegmenter-wiki-51200.model
#ifndef TINYSEGMENTER_HPP__
#define TINYSEGMENTER_HPP__

#include <string>
#include <vector>

class TinySegmenter {
protected:
    int UP1(const std::string &s) const { if(s.size()==1&&s[0]=='U')return -390;else return 0;}
int UW1(const std::string &s) const { if(s.size()<=0)return 0;switch(s[0]){case '\x28':return 144;case '\xe3':if(s.size()<=1)return 0;switch(s[1]){case '\x80':if(s.size()==1&&s[2]=='\x81')return -157;else return 0;case '\x81':if(s.size()<=2)return 0;switch(s[2]){case '\xab':return 162;case '\xaf':return -666;default:return 0;}case '\x82':if(s.size()==1&&s[2]=='\x92')return -116;else return 0;default:return 0;}default:return 0;}}
int BP1(const std::string &s) const { if(s.size()<=0)return 0;switch(s[0]){case 'B':if(s.size()<=1)return 0;switch(s[1]){case 'B':return 79;case 'O':return -285;default:return 0;}case 'O':if(s.size()<=1)return 0;switch(s[1]){case 'B':return -43;case 'O':return 206;default:return 0;}default:return 0;}}
int TW2(const std::string &s) const { if(s.size()<=0)return 0;switch(s[0]){case '\xe3':if(s.size()<=1)return 0;switch(s[1]){case '\x81':if(s.size()<=2)return 0;switch(s[2]){case '\x8a':if(s.size()==3&&s[3]=='\xe3'&&s[4]=='\x81'&&s[5]=='\x91'&&s[6]=='\xe3'&&s[7]=='\x82'&&s[8]=='\x8b')return 221;else return 0;case '\xa6':if(s.size()==3&&s[3]=='\xe3'&&s[4]=='\x81'&&s[5]=='\x84'&&s[6]=='\xe3'&&s[7]=='\x81'&&s[8]=='\x9f')return 1016;else return 0;case '\xa8':if(s.size()==3&&s[3]=='\xe3'&&s[4]=='\x81'&&s[5]=='\x97'&&s[6]=='\xe3'&&s[7]=='\x81'&&s[8]=='\xa6')return 2011;else return 0;default:return 0;}default:return 0;}default:return 0;}}
int TC1(const std::string &s) const { if(s.size()<=0)return 0;switch(s[0]){case 'A':if(s.size()==3&&s[1]=='A'&&s[2]=='O')return -355;else return 0;case 'H':if(s.size()<=1)return 0;switch(s[1]){case 'H':if(s.size()<=2)return 0;switch(s[2]){case 'I':return 692;case 'M':return -296;default:return 0;}case 'I':if(s.size()<=2)return 0;switch(s[2]){case 'H':return -101;case 'I':return 671;default:return 0;}default:return 0;}case 'I':if(s.size()<=1)return 0;switch(s[1]){case 'H':if(s.size()==3&&s[2]=='I')return 82;else return 0;case 'O':if(s.size()==3&&s[2]=='I')return -1691;else return 0;default:return 0;}case 'O':if(s.size()<=1)return 0;switch(s[1]){case 'I':if(s.size()==3&&s[2]=='I')return -185;else return 0;case 'K':if(s.size()==3&&s[2]=='K')return 863;else return 0;default:return 0;}default:return 0;}}
int BC1(const std::string &s) const { if(s.size()<=0)return 0;switch(s[0]){case 'A':if(s.size()==2&&s[1]=='O')return -773;else return 0;case 'H':if(s.size()<=1)return 0;switch(s[1]){case 'I':return 472;case 'K':return -833;default:return 0;}case 'I':if(s.size()<=1)return 0;switch(s[1]){case 'H':return -1403;case 'I':return 1670;case 'O':return 3934;default:return 0;}case 'K':if(s.size()==2&&s[1]=='O')return -200;else return 0;case 'M':if(s.size()==2&&s[1]=='H')return 805;else return 0;case 'N':if(s.size()<=1)return 0;switch(s[1]){case 'H':return 4698;case 'I':return -47;default:return 0;}case 'O':if(s.size()<=1)return 0;switch(s[1]){case 'H':return -910;case 'O':return 654;default:return 0;}default:return 0;}}
int UC5(const std::string &s) const { if(s.size()<=0)return 0;switch(s[0]){case 'H':return 1179;case 'I':return -123;case 'N':return 1265;case 'O':return 61;default:return 0;}}
int BQ3(const std::string &s) const { if(s.size()<=0)return 0;switch(s[0]){case 'B':if(s.size()<=1)return 0;switch(s[1]){case 'H':if(s.size()==3&&s[2]=='I')return 350;else return 0;case 'I':if(s.size()==3&&s[2]=='O')return 672;else return 0;case 'M':if(s.size()==3&&s[2]=='H')return 2704;else return 0;default:return 0;}case 'O':if(s.size()<=1)return 0;switch(s[1]){case 'H':if(s.size()<=2)return 0;switch(s[2]){case 'H':return 3574;case 'I':return -1346;default:return 0;}case 'I':if(s.size()==3&&s[2]=='I')return 905;else return 0;case 'K':if(s.size()==3&&s[2]=='K')return 1983;else return 0;default:return 0;}case 'U':if(s.size()==3&&s[1]=='O'&&s[2]=='H')return -126;else return 0;default:return 0;}}
int UP3(const std::string &s) const { if(s.size()==1&&s[0]=='O')return 388;else return 0;}
int UW2(const std::string &s) const { if(s.size()<=0)return 0;switch(s[0]){case '\x28':return -319;case '\x29':return 505;case '\xe3':if(s.size()<=1)return 0;switch(s[1]){case '\x80':if(s.size()<=2)return 0;switch(s[2]){case '\x81':return -1192;case '\x82':return -1492;case '\x8d':return 791;default:return 0;}case '\x81':if(s.size()<=2)return 0;switch(s[2]){case '\x8a':return 578;case '\x8b':return 370;case '\x8c':return -920;case '\x93':return 1444;case '\x97':return 379;case '\xa3':return 1461;case '\xaa':return 520;case '\xab':return -1546;default:return 0;}case '\x82':if(s.size()<=2)return 0;switch(s[2]){case '\x82':return -684;case '\x88':return 2465;case '\x8b':return -587;case '\x8c':return 1119;case '\x92':return -2415;case '\xa4':return 207;default:return 0;}case '\x83':if(s.size()<=2)return 0;switch(s[2]){case '\x83':return 1940;case '\xbc':return 665;default:return 0;}default:return 0;}case '\xe5':if(s.size()<=1)return 0;switch(s[1]){case '\x88':if(s.size()==1&&s[2]=='\x9d')return -139;else return 0;case '\x8c':if(s.size()==1&&s[2]=='\x97')return -1489;else return 0;case '\x90':if(s.size()==1&&s[2]=='\x8d')return -793;else return 0;case '\xa4':if(s.size()==1&&s[2]=='\xa7')return -2597;else return 0;case '\xb0':if(s.size()==1&&s[2]=='\x8f')return -1408;else return 0;default:return 0;}case '\xe6':if(s.size()==1&&s[1]=='\x9c'&&s[2]=='\x88')return -271;else return 0;case '\xe7':if(s.size()==1&&s[1]=='\x9b'&&s[2]=='\xae')return -1381;else return 0;case '\xe8':if(s.size()==1&&s[1]=='\xa6'&&s[2]=='\x8b')return -2947;else return 0;default:return 0;}}
int TW1(const std::string &s) const { if(s.size()==3&&s[0]=='\xe3'&&s[1]=='\x81'&&s[2]=='\xab'&&s[3]=='\xe3'&&s[4]=='\x81'&&s[5]=='\x8a'&&s[6]=='\xe3'&&s[7]=='\x81'&&s[8]=='\x91')return 7887;else return 0;}
int BQ1(const std::string &s) const { if(s.size()<=0)return 0;switch(s[0]){case 'B':if(s.size()<=1)return 0;switch(s[1]){case 'I':if(s.size()==3&&s[2]=='I')return 221;else return 0;case 'M':if(s.size()==3&&s[2]=='H')return 149;else return 0;default:return 0;}default:return 0;}}
int TQ3(const std::string &s) const { if(s.size()<=0)return 0;switch(s[0]){case 'B':if(s.size()<=1)return 0;switch(s[1]){case 'I':if(s.size()<=2)return 0;switch(s[2]){case 'I':if(s.size()==4&&s[3]=='I')return 691;else return 0;case 'O':if(s.size()==4&&s[3]=='I')return -1204;else return 0;default:return 0;}case 'K':if(s.size()==4&&s[2]=='K'&&s[3]=='I')return 46;else return 0;case 'O':if(s.size()==4&&s[2]=='H'&&s[3]=='H')return 633;else return 0;default:return 0;}case 'O':if(s.size()<=1)return 0;switch(s[1]){case 'A':if(s.size()==4&&s[2]=='A'&&s[3]=='A')return 1217;else return 0;case 'H':if(s.size()<=2)return 0;switch(s[2]){case 'I':if(s.size()<=3)return 0;switch(s[3]){case 'H':return -1150;case 'I':return 1116;default:return 0;}default:return 0;}case 'I':if(s.size()==4&&s[2]=='I'&&s[3]=='I')return -47;else return 0;case 'K':if(s.size()==4&&s[2]=='K'&&s[3]=='K')return 787;else return 0;case 'O':if(s.size()==4&&s[2]=='I'&&s[3]=='I')return -1210;else return 0;default:return 0;}default:return 0;}}
int UW3(const std::string &s) const { if(s.size()<=0)return 0;switch(s[0]){case '\x22':return -1221;case '1':return -2640;case 'e':return 541;case 'i':return -1322;case 's':return 400;case '\xe3':if(s.size()<=1)return 0;switch(s[1]){case '\x80':if(s.size()==1&&s[2]=='\x81')return 3629;else return 0;case '\x81':if(s.size()<=2)return 0;switch(s[2]){case '\x82':return -3714;case '\x84':return 333;case '\x86':return 1146;case '\x8a':return -2494;case '\x8b':return -4344;case '\x8c':return 1446;case '\x8d':return -96;case '\x91':return 1159;case '\x93':return -7890;case '\x99':return -1185;case '\x9d':return -7416;case '\x9f':return 948;case '\xa3':return 924;case '\xa6':return 7996;case '\xa7':return 3430;case '\xa8':return 4459;case '\xaa':return -6179;case '\xab':return 8430;case '\xae':return 4741;case '\xaf':return 4243;case '\xbb':return -1292;case '\xbe':return -5628;default:return 0;}case '\x82':if(s.size()<=2)return 0;switch(s[2]){case '\x88':return -3122;case '\x89':return -436;case '\x8b':return 6034;case '\x8c':return 4431;case '\x92':return 6952;case '\xb0':return 683;case '\xb7':return -1168;case '\xb9':return 202;default:return 0;}case '\x83':if(s.size()<=2)return 0;switch(s[2]){case '\x83':return -2056;case '\x88':return 961;case '\x89':return 1221;case '\xa0':return 1781;case '\xab':return 1501;case '\xb3':return 160;case '\xbc':return 450;default:return 0;}default:return 0;}case '\xe4':if(s.size()<=1)return 0;switch(s[1]){case '\xb8':if(s.size()<=2)return 0;switch(s[2]){case '\x80':return -256;case '\x96':return -1027;case '\xbb':return -997;default:return 0;}case '\xbd':if(s.size()==1&&s[2]=='\x95')return 382;else return 0;default:return 0;}case '\xe5':if(s.size()<=1)return 0;switch(s[1]){case '\x88':if(s.size()==1&&s[2]=='\x9d')return 93;else return 0;case '\xad':if(s.size()==1&&s[2]=='\xa6')return 472;else return 0;case '\xb8':if(s.size()==1&&s[2]=='\x82')return 3500;else return 0;case '\xb9':if(s.size()==1&&s[2]=='\xb4')return 1932;else return 0;case '\xbd':if(s.size()<=2)return 0;switch(s[2]){case '\x93':return -45;case '\xbc':return 1197;default:return 0;}case '\xbe':if(s.size()==1&&s[2]=='\x8c')return 992;else return 0;default:return 0;}case '\xe6':if(s.size()<=1)return 0;switch(s[1]){case '\x8c':if(s.size()==1&&s[2]=='\x87')return -1039;else return 0;case '\x95':if(s.size()==1&&s[2]=='\xb0')return 981;else return 0;case '\x9c':if(s.size()==1&&s[2]=='\x88')return 755;else return 0;case '\xb5':if(s.size()==1&&s[2]=='\xb7')return -629;else return 0;default:return 0;}case '\xe7':if(s.size()<=1)return 0;switch(s[1]){case '\x89':if(s.size()==1&&s[2]=='\xb9')return -3338;else return 0;case '\x9a':if(s.size()==1&&s[2]=='\x84')return 3140;else return 0;case '\x9c':if(s.size()==1&&s[2]=='\x8c')return 4834;else return 0;case '\xac':if(s.size()==1&&s[2]=='\xac')return 832;else return 0;default:return 0;}case '\xe8':if(s.size()<=1)return 0;switch(s[1]){case '\x80':if(s.size()==1&&s[2]=='\x85')return 1717;else return 0;case '\x87':if(s.size()==1&&s[2]=='\xaa')return -235;else return 0;default:return 0;}case '\xe9':if(s.size()==1&&s[1]=='\x83'&&s[2]=='\xa8')return 94;else return 0;default:return 0;}}
int UP2(const std::string &s) const { if(s.size()==1&&s[0]=='B')return 431;else return 0;}
int BW1(const std::string &s) const { if(s.size()<=0)return 0;switch(s[0]){case '\xe3':if(s.size()<=1)return 0;switch(s[1]){case '\x81':if(s.size()<=2)return 0;switch(s[2]){case '\x8a':if(s.size()==2&&s[3]=='\xe3'&&s[4]=='\x81'&&s[5]=='\x91')return 2949;else return 0;case '\x8b':if(s.size()==2&&s[3]=='\xe3'&&s[4]=='\x82'&&s[5]=='\x89')return 1857;else return 0;case '\x95':if(s.size()<=3)return 0;switch(s[3]){case '\xe3':if(s.size()<=4)return 0;switch(s[4]){case '\x82':if(s.size()<=5)return 0;switch(s[5]){case '\x89':return -3447;case '\x8c':return 1033;default:return 0;}default:return 0;}default:return 0;}case '\x97':if(s.size()<=3)return 0;switch(s[3]){case '\xe3':if(s.size()<=4)return 0;switch(s[4]){case '\x81':if(s.size()<=5)return 0;switch(s[5]){case '\x9f':return 674;case '\xa6':return 1502;default:return 0;}default:return 0;}default:return 0;}case '\x9d':if(s.size()==2&&s[3]=='\xe3'&&s[4]=='\x81'&&s[5]=='\x97')return -768;else return 0;case '\xa8':if(s.size()==2&&s[3]=='\xe3'&&s[4]=='\x81'&&s[5]=='\x97')return 3897;else return 0;default:return 0;}default:return 0;}default:return 0;}}
int BQ2(const std::string &s) const { if(s.size()<=0)return 0;switch(s[0]){case 'B':if(s.size()<=1)return 0;switch(s[1]){case 'H':if(s.size()<=2)return 0;switch(s[2]){case 'H':return 589;case 'I':return -466;default:return 0;}case 'I':if(s.size()<=2)return 0;switch(s[2]){case 'H':return -1093;case 'I':return -327;default:return 0;}case 'K':if(s.size()==3&&s[2]=='K')return -160;else return 0;default:return 0;}case 'O':if(s.size()<=1)return 0;switch(s[1]){case 'H':if(s.size()<=2)return 0;switch(s[2]){case 'H':return -665;case 'M':return -414;default:return 0;}case 'K':if(s.size()<=2)return 0;switch(s[2]){case 'K':return -2083;case 'O':return -2087;default:return 0;}case 'O':if(s.size()==3&&s[2]=='O')return -540;else return 0;default:return 0;}case 'U':if(s.size()==3&&s[1]=='K'&&s[2]=='K')return 443;else return 0;default:return 0;}}
int UW4(const std::string &s) const { if(s.size()<=0)return 0;switch(s[0]){case '\x22':return -452;case '\x28':return 189;case '1':return 143;case 'C':return 792;case 'M':return 552;case 'S':return 1283;case 'a':return -1134;case 'e':return -3541;case 'i':return -1653;case 'l':return -528;case 'n':return -2532;case 'o':return -92;case 'r':return -2446;case 's':return -140;case 't':return -924;case '\xe3':if(s.size()<=1)return 0;switch(s[1]){case '\x80':if(s.size()<=2)return 0;switch(s[2]){case '\x81':return 3370;case '\x82':return 2092;default:return 0;}case '\x81':if(s.size()<=2)return 0;switch(s[2]){case '\x82':return 8449;case '\x84':return -2917;case '\x86':return -4412;case '\x88':return -2908;case '\x8a':return 1141;case '\x8c':return 6798;case '\x8d':return -4986;case '\x8f':return -5168;case '\x91':return -5544;case '\x93':return 959;case '\x95':return 3276;case '\x9a':return 868;case '\x9d':return 1867;case '\x9f':return 7014;case '\xa0':return 3131;case '\xa1':return -3340;case '\xa3':return -8570;case '\xa6':return 4357;case '\xa7':return 10133;case '\xa8':return 5861;case '\xa9':return -1045;case '\xaa':return 4914;case '\xab':return 8877;case '\xae':return 9315;case '\xaf':return 8906;case '\xb3':return -3358;case '\xb8':return 2067;case '\xbf':return -3326;default:return 0;}case '\x82':if(s.size()<=2)return 0;switch(s[2]){case '\x81':return -6738;case '\x82':return 4383;case '\x84':return 2070;case '\x88':return 1704;case '\x89':return -521;case '\x8a':return -8511;case '\x8b':return -13937;case '\x8f':return -1792;case '\x92':return 14744;case '\x93':return -2077;case '\xa3':return -2898;case '\xab':return 656;case '\xb3':return 628;default:return 0;}case '\x83':if(s.size()<=2)return 0;switch(s[2]){case '\x83':return -3927;case '\x88':return -1013;case '\x95':return 192;case '\x9e':return 472;case '\xa5':return -894;case '\xab':return -2163;case '\xb3':return -7448;case '\xbb':return -2900;case '\xbc':return -3830;default:return 0;}default:return 0;}case '\xe4':if(s.size()<=1)return 0;switch(s[1]){case '\xb8':if(s.size()==1&&s[2]=='\xad')return 142;else return 0;case '\xba':if(s.size()==1&&s[2]=='\xba')return -710;else return 0;default:return 0;}case '\xe5':if(s.size()<=1)return 0;switch(s[1]){case '\x90':if(s.size()==1&&s[2]=='\x88')return -1168;else return 0;case '\xa4':if(s.size()==1&&s[2]=='\xa7')return 2378;else return 0;case '\xad':if(s.size()==1&&s[2]=='\x90')return -2507;else return 0;case '\xb1':if(s.size()==1&&s[2]=='\x8b')return -286;else return 0;case '\xb3':if(s.size()==1&&s[2]=='\xb6')return -544;else return 0;case '\xb7':if(s.size()==1&&s[2]=='\x9d')return -2133;else return 0;default:return 0;}case '\xe6':if(s.size()==1&&s[1]=='\xa5'&&s[2]=='\xad')return -235;else return 0;case '\xe7':if(s.size()<=1)return 0;switch(s[1]){case '\x94':if(s.size()==1&&s[2]=='\xb0')return -2260;else return 0;case '\x9a':if(s.size()==1&&s[2]=='\x84')return 1349;else return 0;default:return 0;}case '\xe8':if(s.size()==1&&s[1]=='\x80'&&s[2]=='\x85')return 1366;else return 0;case '\xe9':if(s.size()<=1)return 0;switch(s[1]){case '\x81':if(s.size()==1&&s[2]=='\x93')return -747;else return 0;case '\x83':if(s.size()==1&&s[2]=='\x8e')return -1706;else return 0;case '\x96':if(s.size()==1&&s[2]=='\x93')return -827;else return 0;default:return 0;}default:return 0;}}
int UC1(const std::string &s) const { if(s.size()==1&&s[0]=='O')return -674;else return 0;}
int UC4(const std::string &s) const { if(s.size()<=0)return 0;switch(s[0]){case 'H':return 782;case 'I':return 324;case 'M':return -43;case 'N':return 2221;case 'O':return 3619;default:return 0;}}
int BC2(const std::string &s) const { if(s.size()<=0)return 0;switch(s[0]){case 'A':if(s.size()==2&&s[1]=='A')return -13169;else return 0;case 'H':if(s.size()<=1)return 0;switch(s[1]){case 'H':return -3345;case 'I':return -2569;case 'N':return 3771;case 'O':return 6502;default:return 0;}case 'I':if(s.size()<=1)return 0;switch(s[1]){case 'H':return 1975;case 'I':return -7766;case 'O':return 3535;default:return 0;}case 'K':if(s.size()<=1)return 0;switch(s[1]){case 'I':return 1715;case 'K':return -18707;default:return 0;}case 'M':if(s.size()==2&&s[1]=='H')return -5030;else return 0;case 'N':if(s.size()<=1)return 0;switch(s[1]){case 'H':return 3839;case 'N':return -25386;default:return 0;}case 'O':if(s.size()<=1)return 0;switch(s[1]){case 'H':return 1695;case 'O':return -7021;default:return 0;}default:return 0;}}
int TQ4(const std::string &s) const { if(s.size()<=0)return 0;switch(s[0]){case 'B':if(s.size()<=1)return 0;switch(s[1]){case 'H':if(s.size()<=2)return 0;switch(s[2]){case 'H':if(s.size()<=3)return 0;switch(s[3]){case 'H':return 118;case 'I':return -151;case 'M':return -1393;default:return 0;}default:return 0;}case 'O':if(s.size()==4&&s[2]=='H'&&s[3]=='H')return 97;else return 0;default:return 0;}case 'O':if(s.size()<=1)return 0;switch(s[1]){case 'H':if(s.size()<=2)return 0;switch(s[2]){case 'I':if(s.size()<=3)return 0;switch(s[3]){case 'H':return -6076;case 'I':return 421;default:return 0;}default:return 0;}case 'I':if(s.size()<=2)return 0;switch(s[2]){case 'H':if(s.size()==4&&s[3]=='I')return -900;else return 0;case 'I':if(s.size()==4&&s[3]=='I')return -358;else return 0;default:return 0;}case 'K':if(s.size()==4&&s[2]=='O'&&s[3]=='K')return -11237;else return 0;case 'N':if(s.size()<=2)return 0;switch(s[2]){case 'N':if(s.size()<=3)return 0;switch(s[3]){case 'H':return 2679;case 'N':return -4789;default:return 0;}default:return 0;}default:return 0;}default:return 0;}}
int BW2(const std::string &s) const { if(s.size()<=0)return 0;switch(s[0]){case '1':if(s.size()==2&&s[1]=='\xe6'&&s[2]=='\x97'&&s[3]=='\xa5')return -6052;else return 0;case '2':if(s.size()==2&&s[1]=='\xe4'&&s[2]=='\xba'&&s[3]=='\xba')return -1782;else return 0;case '\xe3':if(s.size()<=1)return 0;switch(s[1]){case '\x81':if(s.size()<=2)return 0;switch(s[2]){case '\x84':if(s.size()<=3)return 0;switch(s[3]){case '\xe3':if(s.size()<=4)return 0;switch(s[4]){case '\x81':if(s.size()<=5)return 0;switch(s[5]){case '\x9f':return 1036;case '\xa6':return 1836;case '\xaf':return -92;default:return 0;}default:return 0;}default:return 0;}case '\x8b':if(s.size()<=3)return 0;switch(s[3]){case '\xe3':if(s.size()<=4)return 0;switch(s[4]){case '\x82':if(s.size()<=5)return 0;switch(s[5]){case '\x89':return -6585;case '\x8c':return 956;default:return 0;}default:return 0;}default:return 0;}case '\x91':if(s.size()==2&&s[3]=='\xe3'&&s[4]=='\x82'&&s[5]=='\x8b')return 664;else return 0;case '\x93':if(s.size()<=3)return 0;switch(s[3]){case '\xe3':if(s.size()<=4)return 0;switch(s[4]){case '\x81':if(s.size()<=5)return 0;switch(s[5]){case '\xa8':return -5508;case '\xae':return -1912;default:return 0;}default:return 0;}default:return 0;}case '\x95':if(s.size()==2&&s[3]=='\xe3'&&s[4]=='\x82'&&s[5]=='\x8c')return 12381;else return 0;case '\x97':if(s.size()<=3)return 0;switch(s[3]){case '\xe3':if(s.size()<=4)return 0;switch(s[4]){case '\x81':if(s.size()<=5)return 0;switch(s[5]){case '\x9f':return 3437;case '\xa6':return 3886;default:return 0;}default:return 0;}default:return 0;}case '\x9d':if(s.size()==2&&s[3]=='\xe3'&&s[4]=='\x81'&&s[5]=='\xae')return -3684;else return 0;case '\xa3':if(s.size()<=3)return 0;switch(s[3]){case '\xe3':if(s.size()<=4)return 0;switch(s[4]){case '\x81':if(s.size()<=5)return 0;switch(s[5]){case '\x9f':return 1694;case '\xa6':return 2383;default:return 0;}default:return 0;}default:return 0;}case '\xa6':if(s.size()==2&&s[3]=='\xe3'&&s[4]=='\x81'&&s[5]=='\x84')return 7479;else return 0;case '\xa7':if(s.size()==2&&s[3]=='\xe3'&&s[4]=='\x81'&&s[5]=='\x8d')return -2071;else return 0;case '\xa8':if(s.size()<=3)return 0;switch(s[3]){case '\xe3':if(s.size()<=4)return 0;switch(s[4]){case '\x81':if(s.size()<=5)return 0;switch(s[5]){case '\x84':return 6534;case '\x97':return 1939;default:return 0;}case '\x82':if(s.size()==2&&s[5]=='\x82')return -4414;else return 0;default:return 0;}default:return 0;}case '\xab':if(s.size()==2&&s[3]=='\xe3'&&s[4]=='\x81'&&s[5]=='\xa6')return -9882;else return 0;case '\xbe':if(s.size()<=3)return 0;switch(s[3]){case '\xe3':if(s.size()<=4)return 0;switch(s[4]){case '\x81':if(s.size()<=5)return 0;switch(s[5]){case '\x9f':return -802;case '\xa7':return -8542;default:return 0;}case '\x82':if(s.size()==2&&s[5]=='\x8c')return 485;else return 0;default:return 0;}default:return 0;}default:return 0;}case '\x82':if(s.size()<=2)return 0;switch(s[2]){case '\x81':if(s.size()==2&&s[3]=='\xe3'&&s[4]=='\x81'&&s[5]=='\xa6')return -45;else return 0;case '\x82':if(s.size()==2&&s[3]=='\xe3'&&s[4]=='\x81'&&s[5]=='\xae')return -11211;else return 0;case '\x89':if(s.size()==2&&s[3]=='\xe3'&&s[4]=='\x82'&&s[5]=='\x8c')return -1269;else return 0;case '\x8c':if(s.size()==2&&s[3]=='\xe3'&&s[4]=='\x81'&&s[5]=='\x9f')return 797;else return 0;case '\x8f':if(s.size()==2&&s[3]=='\xe3'&&s[4]=='\x82'&&s[5]=='\x8c')return 2728;else return 0;default:return 0;}default:return 0;}case '\xe6':if(s.size()==2&&s[1]=='\x97'&&s[2]=='\xa5'&&s[3]=='\xe6'&&s[4]=='\x9c'&&s[5]=='\xac')return -573;else return 0;default:return 0;}}
int BP2(const std::string &s) const { if(s.size()<=0)return 0;switch(s[0]){case 'B':if(s.size()<=1)return 0;switch(s[1]){case 'B':return -1829;case 'O':return -173;default:return 0;}case 'O':if(s.size()==2&&s[1]=='O')return 295;else return 0;case 'U':if(s.size()==2&&s[1]=='U')return -609;else return 0;default:return 0;}}
int TC3(const std::string &s) const { if(s.size()<=0)return 0;switch(s[0]){case 'A':if(s.size()==3&&s[1]=='A'&&s[2]=='A')return 1060;else return 0;case 'H':if(s.size()<=1)return 0;switch(s[1]){case 'H':if(s.size()<=2)return 0;switch(s[2]){case 'K':return -158;case 'N':return -248;default:return 0;}case 'I':if(s.size()<=2)return 0;switch(s[2]){case 'H':return 583;case 'I':return -2459;default:return 0;}default:return 0;}case 'I':if(s.size()<=1)return 0;switch(s[1]){case 'H':if(s.size()<=2)return 0;switch(s[2]){case 'H':return 633;case 'I':return -2601;default:return 0;}case 'I':if(s.size()==3&&s[2]=='H')return -1016;else return 0;default:return 0;}case 'K':if(s.size()<=1)return 0;switch(s[1]){case 'K':if(s.size()==3&&s[2]=='O')return 156;else return 0;case 'O':if(s.size()==3&&s[2]=='K')return -4252;else return 0;default:return 0;}case 'M':if(s.size()==3&&s[1]=='H'&&s[2]=='H')return -545;else return 0;case 'N':if(s.size()==3&&s[1]=='N'&&s[2]=='H')return -1157;else return 0;default:return 0;}}
int TQ2(const std::string &s) const { if(s.size()<=0)return 0;switch(s[0]){case 'B':if(s.size()<=1)return 0;switch(s[1]){case 'H':if(s.size()==4&&s[2]=='H'&&s[3]=='H')return -43;else return 0;case 'I':if(s.size()==4&&s[2]=='I'&&s[3]=='I')return -316;else return 0;default:return 0;}case 'O':if(s.size()<=1)return 0;switch(s[1]){case 'H':if(s.size()==4&&s[2]=='H'&&s[3]=='H')return 123;else return 0;case 'K':if(s.size()<=2)return 0;switch(s[2]){case 'H':if(s.size()==4&&s[3]=='H')return -946;else return 0;case 'K':if(s.size()==4&&s[3]=='H')return 1896;else return 0;default:return 0;}default:return 0;}default:return 0;}}
int UW5(const std::string &s) const { if(s.size()<=0)return 0;switch(s[0]){case '\x29':return -45;case 'a':return 48;case 'o':return 672;case '\xe3':if(s.size()<=1)return 0;switch(s[1]){case '\x80':if(s.size()<=2)return 0;switch(s[2]){case '\x81':return 230;case '\x82':return -660;default:return 0;}case '\x81':if(s.size()<=2)return 0;switch(s[2]){case '\x82':return 1549;case '\x84':return 1922;case '\x8b':return 465;case '\x8c':return -171;case '\x95':return -1035;case '\x97':return -1562;case '\x99':return -700;case '\x9f':return 517;case '\xa1':return 141;case '\xa7':return -430;case '\xab':return -797;default:return 0;}case '\x82':if(s.size()<=2)return 0;switch(s[2]){case '\x8b':return 974;case '\x92':return -934;case '\xa4':return 1133;default:return 0;}case '\x83':if(s.size()<=2)return 0;switch(s[2]){case '\x83':return 687;case '\x88':return -749;case '\xa9':return 745;case '\xb3':return 176;case '\xbc':return 651;default:return 0;}default:return 0;}case '\xe5':if(s.size()<=1)return 0;switch(s[1]){case '\xa4':if(s.size()==1&&s[2]=='\xa7')return -300;else return 0;case '\xb8':if(s.size()==1&&s[2]=='\x82')return -3723;else return 0;case '\xbe':if(s.size()==1&&s[2]=='\x8c')return -451;else return 0;default:return 0;}case '\xe6':if(s.size()==1&&s[1]=='\xa0'&&s[2]=='\xa1')return 885;else return 0;case '\xe7':if(s.size()<=1)return 0;switch(s[1]){case '\x94':if(s.size()==1&&s[2]=='\xba')return -703;else return 0;case '\x9a':if(s.size()==1&&s[2]=='\x84')return -2573;else return 0;case '\x9c':if(s.size()==1&&s[2]=='\x8c')return -1429;else return 0;default:return 0;}case '\xe8':if(s.size()==1&&s[1]=='\x80'&&s[2]=='\x85')return -879;else return 0;case '\xe9':if(s.size()==1&&s[1]=='\xa7'&&s[2]=='\x85')return -795;else return 0;default:return 0;}}
int UC6(const std::string &s) const { if(s.size()<=0)return 0;switch(s[0]){case 'I':return 43;case 'K':return 306;default:return 0;}}
int BC3(const std::string &s) const { if(s.size()<=0)return 0;switch(s[0]){case 'A':if(s.size()==2&&s[1]=='A')return 1064;else return 0;case 'H':if(s.size()<=1)return 0;switch(s[1]){case 'H':return 2627;case 'I':return -210;case 'M':return 47;default:return 0;}case 'I':if(s.size()==2&&s[1]=='I')return 581;else return 0;case 'K':if(s.size()<=1)return 0;switch(s[1]){case 'H':return -361;case 'I':return -146;case 'K':return 3935;default:return 0;}case 'N':if(s.size()==2&&s[1]=='N')return 946;else return 0;case 'O':if(s.size()<=1)return 0;switch(s[1]){case 'A':return -1480;case 'H':return 921;case 'I':return 1317;default:return 0;}default:return 0;}}
int UQ3(const std::string &s) const { if(s.size()<=0)return 0;switch(s[0]){case 'B':if(s.size()<=1)return 0;switch(s[1]){case 'H':return 285;case 'I':return 5441;case 'K':return -1498;case 'O':return 9249;default:return 0;}case 'O':if(s.size()<=1)return 0;switch(s[1]){case 'H':return 357;case 'N':return 2600;case 'O':return -4401;default:return 0;}default:return 0;}}
int UQ1(const std::string &s) const { if(s.size()<=0)return 0;switch(s[0]){case 'B':if(s.size()<=1)return 0;switch(s[1]){case 'H':return -57;case 'K':return 100;case 'N':return -55;default:return 0;}case 'O':if(s.size()<=1)return 0;switch(s[1]){case 'K':return -188;case 'N':return 132;default:return 0;}default:return 0;}}
int TQ1(const std::string &s) const { if(s.size()<=0)return 0;switch(s[0]){case 'B':if(s.size()<=1)return 0;switch(s[1]){case 'I':if(s.size()==4&&s[2]=='I'&&s[3]=='I')return 859;else return 0;case 'O':if(s.size()==4&&s[2]=='K'&&s[3]=='K')return 145;else return 0;default:return 0;}case 'O':if(s.size()<=1)return 0;switch(s[1]){case 'I':if(s.size()==4&&s[2]=='I'&&s[3]=='H')return 105;else return 0;case 'K':if(s.size()==4&&s[2]=='K'&&s[3]=='K')return 2109;else return 0;default:return 0;}default:return 0;}}
int TC2(const std::string &s) const { if(s.size()<=0)return 0;switch(s[0]){case 'H':if(s.size()<=1)return 0;switch(s[1]){case 'H':if(s.size()<=2)return 0;switch(s[2]){case 'H':return -1124;case 'I':return 1048;case 'M':return -442;default:return 0;}default:return 0;}case 'I':if(s.size()<=1)return 0;switch(s[1]){case 'H':if(s.size()==3&&s[2]=='I')return -3483;else return 0;case 'I':if(s.size()==3&&s[2]=='I')return -2181;else return 0;default:return 0;}case 'N':if(s.size()==3&&s[1]=='I'&&s[2]=='H')return -4102;else return 0;case 'O':if(s.size()<=1)return 0;switch(s[1]){case 'H':if(s.size()==3&&s[2]=='I')return -1554;else return 0;case 'I':if(s.size()==3&&s[2]=='I')return -1425;else return 0;default:return 0;}default:return 0;}}
int BQ4(const std::string &s) const { if(s.size()<=0)return 0;switch(s[0]){case 'B':if(s.size()<=1)return 0;switch(s[1]){case 'A':if(s.size()==3&&s[2]=='A')return -3479;else return 0;case 'H':if(s.size()<=2)return 0;switch(s[2]){case 'H':return -7226;case 'M':return -622;default:return 0;}case 'I':if(s.size()<=2)return 0;switch(s[2]){case 'H':return 2747;case 'I':return -5715;default:return 0;}case 'O':if(s.size()==3&&s[2]=='O')return -4127;else return 0;default:return 0;}case 'O':if(s.size()<=1)return 0;switch(s[1]){case 'I':if(s.size()==3&&s[2]=='H')return -1450;else return 0;case 'N':if(s.size()==3&&s[2]=='H')return 723;else return 0;default:return 0;}case 'U':if(s.size()==3&&s[1]=='H'&&s[2]=='H')return -4325;else return 0;default:return 0;}}
int TW4(const std::string &s) const { if(s.size()<=0)return 0;switch(s[0]){default:return 0;}}
int TC4(const std::string &s) const { if(s.size()<=0)return 0;switch(s[0]){case 'A':if(s.size()==3&&s[1]=='A'&&s[2]=='A')return 1614;else return 0;case 'H':if(s.size()<=1)return 0;switch(s[1]){case 'H':if(s.size()<=2)return 0;switch(s[2]){case 'H':return -1492;case 'I':return 649;case 'O':return 112;default:return 0;}case 'I':if(s.size()<=2)return 0;switch(s[2]){case 'H':return 269;case 'I':return -429;default:return 0;}case 'K':if(s.size()==3&&s[2]=='K')return -742;else return 0;default:return 0;}case 'I':if(s.size()<=1)return 0;switch(s[1]){case 'I':if(s.size()<=2)return 0;switch(s[2]){case 'H':return -348;case 'I':return 1048;case 'O':return 459;default:return 0;}default:return 0;}case 'K':if(s.size()==3&&s[1]=='K'&&s[2]=='K')return 2066;else return 0;default:return 0;}}
int UW6(const std::string &s) const { if(s.size()<=0)return 0;switch(s[0]){case '\xe3':if(s.size()<=1)return 0;switch(s[1]){case '\x80':if(s.size()==1&&s[2]=='\x82')return 774;else return 0;case '\x81':if(s.size()<=2)return 0;switch(s[2]){case '\x9f':return -204;case '\xa8':return -150;default:return 0;}case '\x82':if(s.size()<=2)return 0;switch(s[2]){case '\x8b':return 560;case '\x8c':return -506;default:return 0;}default:return 0;}case '\xe5':if(s.size()==1&&s[1]=='\xb9'&&s[2]=='\xb4')return -833;else return 0;default:return 0;}}
int BW3(const std::string &s) const { if(s.size()<=0)return 0;switch(s[0]){case '\xe3':if(s.size()<=1)return 0;switch(s[1]){case '\x81':if(s.size()<=2)return 0;switch(s[2]){case '\x82':if(s.size()==2&&s[3]=='\xe3'&&s[4]=='\x82'&&s[5]=='\x8b')return 92;else return 0;case '\x84':if(s.size()<=3)return 0;switch(s[3]){case '\xe3':if(s.size()<=4)return 0;switch(s[4]){case '\x81':if(s.size()==2&&s[5]=='\x86')return 2160;else return 0;case '\x82':if(s.size()==2&&s[5]=='\x8b')return 1519;else return 0;default:return 0;}default:return 0;}case '\x8b':if(s.size()==2&&s[3]=='\xe3'&&s[4]=='\x82'&&s[5]=='\x89')return 8293;else return 0;case '\x8c':if(s.size()==2&&s[3]=='\xe3'&&s[4]=='\x80'&&s[5]=='\x81')return 2507;else return 0;case '\x93':if(s.size()==2&&s[3]=='\xe3'&&s[4]=='\x81'&&s[5]=='\xa8')return 6358;else return 0;case '\x97':if(s.size()<=3)return 0;switch(s[3]){case '\xe3':if(s.size()<=4)return 0;switch(s[4]){case '\x81':if(s.size()<=5)return 0;switch(s[5]){case '\x9f':return 1027;case '\xa6':return 2197;default:return 0;}default:return 0;}default:return 0;}case '\x99':if(s.size()==2&&s[3]=='\xe3'&&s[4]=='\x82'&&s[5]=='\x8b')return 2963;else return 0;case '\x9f':if(s.size()==2&&s[3]=='\xe3'&&s[4]=='\x80'&&s[5]=='\x82')return 8234;else return 0;case '\xa3':if(s.size()==2&&s[3]=='\xe3'&&s[4]=='\x81'&&s[5]=='\x9f')return -1007;else return 0;case '\xa6':if(s.size()==2&&s[3]=='\xe3'&&s[4]=='\x81'&&s[5]=='\x84')return 3772;else return 0;case '\xa8':if(s.size()==2&&s[3]=='\xe3'&&s[4]=='\x81'&&s[5]=='\x97')return 1428;else return 0;case '\xbe':if(s.size()==2&&s[3]=='\xe3'&&s[4]=='\x81'&&s[5]=='\xa7')return 2179;else return 0;default:return 0;}case '\x82':if(s.size()<=2)return 0;switch(s[2]){case '\x89':if(s.size()==2&&s[3]=='\xe3'&&s[4]=='\x82'&&s[5]=='\x8c')return 4056;else return 0;case '\x8c':if(s.size()==2&&s[3]=='\xe3'&&s[4]=='\x81'&&s[5]=='\xa6')return 370;else return 0;default:return 0;}case '\x83':if(s.size()==2&&s[2]=='\xbc'&&s[3]=='\xe3'&&s[4]=='\x83'&&s[5]=='\xbb')return 3127;else return 0;default:return 0;}case '\xe5':if(s.size()==2&&s[1]=='\xad'&&s[2]=='\xa6'&&s[3]=='\xe6'&&s[4]=='\xa0'&&s[5]=='\xa1')return 233;else return 0;case '\xe6':if(s.size()==2&&s[1]=='\x97'&&s[2]=='\xa5'&&s[3]=='\xe6'&&s[4]=='\x9c'&&s[5]=='\xac')return 1112;else return 0;default:return 0;}}
int UC2(const std::string &s) const { if(s.size()<=0)return 0;switch(s[0]){case 'H':return 2641;case 'M':return 1796;case 'N':return 2566;case 'O':return -338;default:return 0;}}
int UC3(const std::string &s) const { if(s.size()<=0)return 0;switch(s[0]){case 'I':return 1644;case 'M':return -202;case 'N':return 1870;default:return 0;}}
int UQ2(const std::string &s) const { if(s.size()<=0)return 0;switch(s[0]){case 'B':if(s.size()<=1)return 0;switch(s[1]){case 'H':return -84;case 'O':return 1181;default:return 0;}case 'O':if(s.size()<=1)return 0;switch(s[1]){case 'H':return -44;case 'K':return 1288;default:return 0;}case 'U':if(s.size()==2&&s[1]=='O')return -297;else return 0;default:return 0;}}
int TW3(const std::string &s) const { if(s.size()==3&&s[0]=='\xe3'&&s[1]=='\x81'&&s[2]=='\xa8'&&s[3]=='\xe3'&&s[4]=='\x81'&&s[5]=='\x97'&&s[6]=='\xe3'&&s[7]=='\x81'&&s[8]=='\xa6')return 1206;else return 0;}
int BIAS() const {return 3303;}

    static unsigned int lengthOfCharactor(int ch) {
        if((ch & 0x80) == 0) {
            return 1;
        } else if((ch & 0xE0) == 0xC0) {
            return 2;
        } else if((ch & 0xF0) == 0xE0) {
            return 3;
        } else if((ch & 0xF8) == 0xF0) {
            return 4;
        }
        return 0;
    }

    static const char* ctype_(const std::string& ch) {
        // [一二三四五六七八九十百千万億兆]
        if(ch.size() == 3 && (
               ch[0] == '\xe4' && (
                   ch[1] == '\xb8' && (ch[2] == '\x80' || ch[2] == '\x83' || ch[2] == '\x87' || ch[2] == '\x89') ||
                   ch[1] == '\xb9' && ch[2] == '\x9d' ||
                   ch[1] == '\xba' && ch[2] == '\x94') ||
               ch[0] == '\xe5' && (
                   ch[1] == '\x84' && ch[2] == '\x84' ||
                   ch[1] == '\x85' && (ch[2] == '\x86' || ch[2] == '\xab' || ch[2] == '\xad') ||
                   ch[1] == '\x8d' && (ch[2] == '\x81' || ch[2] == '\x83') ||
                   ch[1] == '\x9b' && ch[2] == '\x9b') ||
               ch[0] == '\xe7' && ch[1] == '\x99' && ch[2] == '\xbe')) return "M";

        // [一-龠々〆ヵヶ]
        if("\xe4\xb8\x80" <= ch && ch <= "\xe9\xbe\xa0" ||
               ch.size() == 3 && ch[0] == '\xe3' && (
                       ch[1] == '\x80' && (ch[2] == '\x85' || ch[2] == '\x86') ||
                       ch[1] == '\x83' && (ch[2] == '\xb5' || ch[2] == '\xb6'))) return "H";

        // [ぁ-ん]
        if("\xe3\x81\x81" <= ch && ch <= "\xe3\x82\x93") return "I";

        //[ァ-ヴーｱ-ﾝﾞｰ]
        if("\xe3\x82\xa1" <= ch && ch <= "\xe3\x83\xb4" || ch == "\xe3\x83\xbc" ||
           "\xef\xbd\xb1" <= ch && ch <= "\xef\xbe\x9d" || ch == "\xef\xbe\x9e" || ch == "\xef\xbd\xb0") return "K";

        //[a-zA-Zａ-ｚＡ-Ｚ]
        if("a" <= ch && ch <= "z" ||
           "A" <= ch && ch <= "Z" ||
           "\xef\xbd\x81" <= ch && ch <= "\xef\xbd\x9a" ||
           "\xef\xbc\xa1" <= ch && ch <= "\xef\xbc\xba") return "A";

        //[0-9０-９]
        if("0" <= ch && ch <= "9" ||
           "\xef\xbc\x90" <= ch && ch <= "\xef\xbc\x99") return "N";

        return "O";
    }

    std::vector<std::string> result, ctype, seg;
    std::string word;
    std::string tmp;

public:
    std::vector<std::string> segment(const std::string& s) {
        if(s.empty()) return std::vector<std::string>();
        result.resize(0);
        ctype.resize(0);
        seg.resize(0);

        seg.push_back("B3"); ctype.push_back("O");
        seg.push_back("B2"); ctype.push_back("O");
        seg.push_back("B1"); ctype.push_back("O");
        for(unsigned int i = 0; i < s.size();) {
            unsigned int len = lengthOfCharactor(s[i]);
            if(len==0) throw "Invalid Charactor";
            std::string ch = s.substr(i, len);
            seg.push_back(ch);
            ctype.push_back(ctype_(ch));
            i += len;
        }
        seg.push_back("E1"); ctype.push_back("O");
        seg.push_back("E2"); ctype.push_back("O");
        seg.push_back("E3"); ctype.push_back("O");

        word = seg[3];
        std::string p1 = "U", p2 = "U", p3 = "U";

        for(unsigned int i = 4; i<seg.size()-3; ++i) {
            int score = BIAS();
            const std::string& w1 = seg[i-3];
            const std::string& w2 = seg[i-2];
            const std::string& w3 = seg[i-1];
            const std::string& w4 = seg[i];
            const std::string& w5 = seg[i+1];
            const std::string& w6 = seg[i+2];
            const std::string& c1 = ctype[i-3];
            const std::string& c2 = ctype[i-2];
            const std::string& c3 = ctype[i-1];
            const std::string& c4 = ctype[i];
            const std::string& c5 = ctype[i+1];
            const std::string& c6 = ctype[i+2];
            score += UP1(p1);
            score += UP2(p2);
            score += UP3(p3);
            tmp = p1; tmp += p2; score += BP1(tmp);
            tmp = p2; tmp += p3; score += BP2(tmp);
            score += UW1(w1);
            score += UW2(w2);
            score += UW3(w3);
            score += UW4(w4);
            score += UW5(w5);
            score += UW6(w6);
            tmp = w1; tmp += w2; tmp += w3; score += TW1(tmp);
            tmp = w2; tmp += w3; score += BW1(tmp);
            tmp += w4; score += TW2(tmp);
            tmp = w3; tmp += w4; score += BW2(tmp);
            tmp += w5; score += TW3(tmp);
            tmp = w4; tmp += w5; score += BW3(tmp);
            tmp += w6; score += TW4(tmp);
            score += UC1(c1);
            score += UC2(c2);
            score += UC3(c3);
            score += UC4(c4);
            score += UC5(c5);
            score += UC6(c6);
            tmp = c1; tmp += c2; tmp += c3; score += TC1(tmp);
            tmp = c2; tmp += c3; score += BC1(tmp);
            tmp += c4; score += TC2(tmp);
            tmp = c3; tmp += c4; score += BC2(tmp);
            tmp += c5; score += TC3(tmp);
            tmp = c4; tmp += c5; score += BC3(tmp);
            tmp += c6; score += TC4(tmp);
            tmp = p1; tmp += c1; score += UQ1(tmp);
            tmp = p2; tmp += c2; score += UQ2(tmp);
            tmp += c3; score += BQ1(tmp);
            tmp += c4; score += TQ2(tmp);
            tmp = p3; tmp += c3; score += UQ3(tmp);
            tmp += c4; score += BQ4(tmp);
            tmp = p2; tmp += c3; tmp += c4; score += BQ2(tmp);
            tmp = p3; tmp += c2; tmp += c3; score += BQ3(tmp);
            tmp += c4; score += TQ4(tmp);
            tmp = p2; tmp += c1; tmp += c2; tmp += c3; score += TQ1(tmp);
            tmp = p3; tmp += c1; tmp += c2; tmp += c3; score += TQ3(tmp);
            std::string p = "O";
            if(score > 0) {
                result.push_back(word);
                word.resize(0);
                p = "B";
            }
            p1 = p2;
            p2 = p3;
            p3 = p;
            word += seg[i];
        }
        result.push_back(word);
        return result;
    }
};

#endif
