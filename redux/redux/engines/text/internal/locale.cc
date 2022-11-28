/*
Copyright 2017-2022 Google Inc. All Rights Reserved.

Licensed under the Apache License, Version 2.0 (the "License");
you may not use this file except in compliance with the License.
You may obtain a copy of the License at

    http://www.apache.org/licenses/LICENSE-2.0

Unless required by applicable law or agreed to in writing, software
distributed under the License is distributed on an "AS-IS" BASIS,
WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
See the License for the specific language governing permissions and
limitations under the License.
*/

#include "redux/engines/text/internal/locale.h"

#include <algorithm>

#include "redux/modules/base/hash.h"
#include "redux/modules/base/logging.h"

#define REDUX_TEXT_LANGUAGE_TUPLE \
  LANG("aa",         "Latn", kLeftToRight) \
  LANG("ab",         "Cyrl", kLeftToRight) \
  LANG("abq",        "Cyrl", kLeftToRight) \
  LANG("ace",        "Latn", kLeftToRight) \
  LANG("ach",        "Latn", kLeftToRight) \
  LANG("ada",        "Latn", kLeftToRight) \
  LANG("ady",        "Cyrl", kLeftToRight) \
  LANG("ae",         "Avst", kRightToLeft) \
  LANG("af",         "Latn", kLeftToRight) \
  LANG("agq",        "Latn", kLeftToRight) \
  LANG("aii",        "Cyrl", kLeftToRight) \
  LANG("ain",        "Kana", kLeftToRight) \
  LANG("ak",         "Latn", kLeftToRight) \
  LANG("akk",        "Xsux", kLeftToRight) \
  LANG("ale",        "Latn", kLeftToRight) \
  LANG("alt",        "Cyrl", kLeftToRight) \
  LANG("am",         "Ethi", kLeftToRight) \
  LANG("amo",        "Latn", kLeftToRight) \
  LANG("an",         "Latn", kLeftToRight) \
  LANG("anp",        "Deva", kLeftToRight) \
  LANG("ar",         "Arab", kRightToLeft) \
  LANG("ar-IR",      "Syrc", kRightToLeft) \
  LANG("arc",        "Armi", kRightToLeft) \
  LANG("arn",        "Latn", kLeftToRight) \
  LANG("arp",        "Latn", kLeftToRight) \
  LANG("arw",        "Latn", kLeftToRight) \
  LANG("as",         "Beng", kLeftToRight) \
  LANG("asa",        "Latn", kLeftToRight) \
  LANG("ast",        "Latn", kLeftToRight) \
  LANG("av",         "Cyrl", kLeftToRight) \
  LANG("awa",        "Deva", kLeftToRight) \
  LANG("ay",         "Latn", kLeftToRight) \
  LANG("az",         "Latn", kLeftToRight) \
  LANG("az-AZ",      "Cyrl", kLeftToRight) \
  LANG("az-IR",      "Arab", kRightToLeft) \
  LANG("ba",         "Cyrl", kLeftToRight) \
  LANG("bal",        "Arab", kRightToLeft) \
  LANG("bal-IR",     "Latn", kLeftToRight) \
  LANG("bal-PK",     "Latn", kLeftToRight) \
  LANG("ban",        "Latn", kLeftToRight) \
  LANG("ban-ID",     "Bali", kLeftToRight) \
  LANG("bas",        "Latn", kLeftToRight) \
  LANG("bax",        "Bamu", kLeftToRight) \
  LANG("bbc",        "Latn", kLeftToRight) \
  LANG("bbc-ID",     "Batk", kLeftToRight) \
  LANG("be",         "Cyrl", kLeftToRight) \
  LANG("bej",        "Arab", kRightToLeft) \
  LANG("bem",        "Latn", kLeftToRight) \
  LANG("bez",        "Latn", kLeftToRight) \
  LANG("bfq",        "Taml", kLeftToRight) \
  LANG("bft",        "Arab", kRightToLeft) \
  LANG("bfy",        "Deva", kLeftToRight) \
  LANG("bg",         "Cyrl", kLeftToRight) \
  LANG("bh",         "Deva", kLeftToRight) \
  LANG("bhb",        "Deva", kLeftToRight) \
  LANG("bho",        "Deva", kLeftToRight) \
  LANG("bi",         "Latn", kLeftToRight) \
  LANG("bik",        "Latn", kLeftToRight) \
  LANG("bin",        "Latn", kLeftToRight) \
  LANG("bjj",        "Deva", kLeftToRight) \
  LANG("bku",        "Latn", kLeftToRight) \
  LANG("bla",        "Latn", kLeftToRight) \
  LANG("blt",        "Tavt", kLeftToRight) \
  LANG("bm",         "Latn", kLeftToRight) \
  LANG("bn",         "Beng", kLeftToRight) \
  LANG("bo",         "Tibt", kLeftToRight) \
  LANG("bqv",        "Latn", kLeftToRight) \
  LANG("br",         "Latn", kLeftToRight) \
  LANG("bra",        "Deva", kLeftToRight) \
  LANG("brx",        "Deva", kLeftToRight) \
  LANG("bs",         "Latn", kLeftToRight) \
  LANG("btv",        "Deva", kLeftToRight) \
  LANG("bua",        "Cyrl", kLeftToRight) \
  LANG("buc",        "Latn", kLeftToRight) \
  LANG("bug",        "Latn", kLeftToRight) \
  LANG("bug-ID",     "Bugi", kLeftToRight) \
  LANG("bya",        "Latn", kLeftToRight) \
  LANG("byn",        "Ethi", kLeftToRight) \
  LANG("ca",         "Latn", kLeftToRight) \
  LANG("cad",        "Latn", kLeftToRight) \
  LANG("car",        "Latn", kLeftToRight) \
  LANG("cay",        "Latn", kLeftToRight) \
  LANG("cch",        "Latn", kLeftToRight) \
  LANG("ccp",        "Beng", kLeftToRight) \
  LANG("ce",         "Cyrl", kLeftToRight) \
  LANG("ceb",        "Latn", kLeftToRight) \
  LANG("cgg",        "Latn", kLeftToRight) \
  LANG("ch",         "Latn", kLeftToRight) \
  LANG("chk",        "Latn", kLeftToRight) \
  LANG("chm",        "Cyrl", kLeftToRight) \
  LANG("chn",        "Latn", kLeftToRight) \
  LANG("cho",        "Latn", kLeftToRight) \
  LANG("chp",        "Latn", kLeftToRight) \
  LANG("chr",        "Cher", kLeftToRight) \
  LANG("chy",        "Latn", kLeftToRight) \
  LANG("cja",        "Arab", kRightToLeft) \
  LANG("cjm",        "Cham", kLeftToRight) \
  LANG("cjs",        "Cyrl", kLeftToRight) \
  LANG("ckb",        "Arab", kRightToLeft) \
  LANG("ckt",        "Cyrl", kLeftToRight) \
  LANG("co",         "Latn", kLeftToRight) \
  LANG("cop",        "Arab", kRightToLeft) \
  LANG("cpe",        "Latn", kLeftToRight) \
  LANG("cr",         "Cans", kLeftToRight) \
  LANG("crh",        "Cyrl", kLeftToRight) \
  LANG("crk",        "Cans", kLeftToRight) \
  LANG("cs",         "Latn", kLeftToRight) \
  LANG("csb",        "Latn", kLeftToRight) \
  LANG("cu",         "Glag", kLeftToRight) \
  LANG("cv",         "Cyrl", kLeftToRight) \
  LANG("cy",         "Latn", kLeftToRight) \
  LANG("da",         "Latn", kLeftToRight) \
  LANG("dak",        "Latn", kLeftToRight) \
  LANG("dar",        "Cyrl", kLeftToRight) \
  LANG("dav",        "Latn", kLeftToRight) \
  LANG("de",         "Latn", kLeftToRight) \
  LANG("de-1901",    "Latn", kLeftToRight) \
  LANG("de-BR",      "Runr", kLeftToRight) \
  LANG("de-CH-1901", "Latn", kLeftToRight) \
  LANG("de-KZ",      "Latn", kLeftToRight) \
  LANG("de-LI-1901", "Latn", kLeftToRight) \
  LANG("de-US",      "Latn", kLeftToRight) \
  LANG("del",        "Latn", kLeftToRight) \
  LANG("den",        "Latn", kLeftToRight) \
  LANG("dgr",        "Latn", kLeftToRight) \
  LANG("din",        "Latn", kLeftToRight) \
  LANG("dje",        "Latn", kLeftToRight) \
  LANG("dng",        "Cyrl", kLeftToRight) \
  LANG("doi",        "Arab", kRightToLeft) \
  LANG("dsb",        "Latn", kLeftToRight) \
  LANG("dua",        "Latn", kLeftToRight) \
  LANG("dv",         "Thaa", kRightToLeft) \
  LANG("dyo",        "Arab", kRightToLeft) \
  LANG("dyu",        "Latn", kLeftToRight) \
  LANG("dz",         "Tibt", kLeftToRight) \
  LANG("ebu",        "Latn", kLeftToRight) \
  LANG("ee",         "Latn", kLeftToRight) \
  LANG("efi",        "Latn", kLeftToRight) \
  LANG("egy",        "Egyp", kLeftToRight) \
  LANG("eka",        "Latn", kLeftToRight) \
  LANG("eky",        "Kali", kLeftToRight) \
  LANG("el",         "Grek", kLeftToRight) \
  LANG("en",         "Latn", kLeftToRight) \
  LANG("en-AS",      "Latn", kLeftToRight) \
  LANG("en-GB",      "Latn", kLeftToRight) \
  LANG("en-GU",      "Latn", kLeftToRight) \
  LANG("en-MH",      "Latn", kLeftToRight) \
  LANG("en-MP",      "Latn", kLeftToRight) \
  LANG("en-PR",      "Latn", kLeftToRight) \
  LANG("en-UM",      "Latn", kLeftToRight) \
  LANG("en-US",      "Latn", kLeftToRight) \
  LANG("en-VI",      "Latn", kLeftToRight) \
  LANG("eo",         "Latn", kLeftToRight) \
  LANG("es",         "Latn", kLeftToRight) \
  LANG("et",         "Latn", kLeftToRight) \
  LANG("ett",        "Ital", kLeftToRight) \
  LANG("eu",         "Latn", kLeftToRight) \
  LANG("evn",        "Cyrl", kLeftToRight) \
  LANG("ewo",        "Latn", kLeftToRight) \
  LANG("fa",         "Arab", kRightToLeft) \
  LANG("fan",        "Latn", kLeftToRight) \
  LANG("ff",         "Latn", kLeftToRight) \
  LANG("fi",         "Latn", kLeftToRight) \
  LANG("fil",        "Latn", kLeftToRight) \
  LANG("fil-US",     "Tglg", kLeftToRight) \
  LANG("fiu",        "Latn", kLeftToRight) \
  LANG("fj",         "Latn", kLeftToRight) \
  LANG("fo",         "Latn", kLeftToRight) \
  LANG("fon",        "Latn", kLeftToRight) \
  LANG("fr",         "Latn", kLeftToRight) \
  LANG("frr",        "Latn", kLeftToRight) \
  LANG("frs",        "Latn", kLeftToRight) \
  LANG("fur",        "Latn", kLeftToRight) \
  LANG("fy",         "Latn", kLeftToRight) \
  LANG("ga",         "Latn", kLeftToRight) \
  LANG("gaa",        "Latn", kLeftToRight) \
  LANG("gag",        "Latn", kLeftToRight) \
  LANG("gag-MD",     "Cyrl", kLeftToRight) \
  LANG("gay",        "Latn", kLeftToRight) \
  LANG("gba",        "Arab", kRightToLeft) \
  LANG("gbm",        "Deva", kLeftToRight) \
  LANG("gcr",        "Latn", kLeftToRight) \
  LANG("gd",         "Latn", kLeftToRight) \
  LANG("gez",        "Ethi", kLeftToRight) \
  LANG("gil",        "Latn", kLeftToRight) \
  LANG("gl",         "Latn", kLeftToRight) \
  LANG("gld",        "Cyrl", kLeftToRight) \
  LANG("gn",         "Latn", kLeftToRight) \
  LANG("gon",        "Telu", kLeftToRight) \
  LANG("gor",        "Latn", kLeftToRight) \
  LANG("got",        "Goth", kLeftToRight) \
  LANG("grb",        "Latn", kLeftToRight) \
  LANG("grc",        "Cprt", kRightToLeft) \
  LANG("grt",        "Beng", kLeftToRight) \
  LANG("gsw",        "Latn", kLeftToRight) \
  LANG("gu",         "Gujr", kLeftToRight) \
  LANG("guz",        "Latn", kLeftToRight) \
  LANG("gv",         "Latn", kLeftToRight) \
  LANG("gwi",        "Latn", kLeftToRight) \
  LANG("ha",         "Arab", kRightToLeft) \
  LANG("ha-GH",      "Latn", kLeftToRight) \
  LANG("ha-NE",      "Latn", kLeftToRight) \
  LANG("hai",        "Latn", kLeftToRight) \
  LANG("haw",        "Latn", kLeftToRight) \
  LANG("he",         "Hebr", kRightToLeft) \
  LANG("hi",         "Deva", kLeftToRight) \
  LANG("hil",        "Latn", kLeftToRight) \
  LANG("hit",        "Xsux", kLeftToRight) \
  LANG("hmn",        "Latn", kLeftToRight) \
  LANG("hne",        "Deva", kLeftToRight) \
  LANG("hnn",        "Latn", kLeftToRight) \
  LANG("ho",         "Latn", kLeftToRight) \
  LANG("hoc",        "Deva", kLeftToRight) \
  LANG("hoj",        "Deva", kLeftToRight) \
  LANG("hop",        "Latn", kLeftToRight) \
  LANG("hr",         "Latn", kLeftToRight) \
  LANG("hsb",        "Latn", kLeftToRight) \
  LANG("ht",         "Latn", kLeftToRight) \
  LANG("hu",         "Latn", kLeftToRight) \
  LANG("hup",        "Latn", kLeftToRight) \
  LANG("hy",         "Armn", kLeftToRight) \
  LANG("hz",         "Latn", kLeftToRight) \
  LANG("ia",         "Latn", kLeftToRight) \
  LANG("iba",        "Latn", kLeftToRight) \
  LANG("ibb",        "Latn", kLeftToRight) \
  LANG("id",         "Latn", kLeftToRight) \
  LANG("ig",         "Latn", kLeftToRight) \
  LANG("ii",         "Yiii", kLeftToRight) \
  LANG("ii-CN",      "Latn", kLeftToRight) \
  LANG("ik",         "Latn", kLeftToRight) \
  LANG("ilo",        "Latn", kLeftToRight) \
  LANG("inh",        "Cyrl", kLeftToRight) \
  LANG("is",         "Latn", kLeftToRight) \
  LANG("it",         "Latn", kLeftToRight) \
  LANG("iu",         "Cans", kLeftToRight) \
  LANG("iu-CA",      "Latn", kLeftToRight) \
  LANG("ja",         "Jpan", kLeftToRight) \
  LANG("jmc",        "Latn", kLeftToRight) \
  LANG("jpr",        "Hebr", kRightToLeft) \
  LANG("jrb",        "Hebr", kRightToLeft) \
  LANG("jv",         "Latn", kLeftToRight) \
  LANG("jv-ID",      "Java", kLeftToRight) \
  LANG("ka",         "Geor", kLeftToRight) \
  LANG("kaa",        "Cyrl", kLeftToRight) \
  LANG("kab",        "Latn", kLeftToRight) \
  LANG("kac",        "Latn", kLeftToRight) \
  LANG("kaj",        "Latn", kLeftToRight) \
  LANG("kam",        "Latn", kLeftToRight) \
  LANG("kbd",        "Cyrl", kLeftToRight) \
  LANG("kca",        "Cyrl", kLeftToRight) \
  LANG("kcg",        "Latn", kLeftToRight) \
  LANG("kde",        "Latn", kLeftToRight) \
  LANG("kdt",        "Thai", kLeftToRight) \
  LANG("kea",        "Latn", kLeftToRight) \
  LANG("kfo",        "Latn", kLeftToRight) \
  LANG("kfr",        "Deva", kLeftToRight) \
  LANG("kg",         "Latn", kLeftToRight) \
  LANG("kha",        "Latn", kLeftToRight) \
  LANG("kha-IN",     "Beng", kLeftToRight) \
  LANG("khb",        "Talu", kLeftToRight) \
  LANG("khq",        "Latn", kLeftToRight) \
  LANG("kht",        "Mymr", kLeftToRight) \
  LANG("ki",         "Latn", kLeftToRight) \
  LANG("kj",         "Latn", kLeftToRight) \
  LANG("kjh",        "Cyrl", kLeftToRight) \
  LANG("kk",         "Arab", kRightToLeft) \
  LANG("kk-KZ",      "Cyrl", kLeftToRight) \
  LANG("kk-TR",      "Cyrl", kLeftToRight) \
  LANG("kl",         "Latn", kLeftToRight) \
  LANG("kln",        "Latn", kLeftToRight) \
  LANG("km",         "Khmr", kLeftToRight) \
  LANG("kmb",        "Latn", kLeftToRight) \
  LANG("kn",         "Knda", kLeftToRight) \
  LANG("ko",         "Kore", kLeftToRight) \
  LANG("koi",        "Cyrl", kLeftToRight) \
  LANG("kok",        "Deva", kLeftToRight) \
  LANG("kos",        "Latn", kLeftToRight) \
  LANG("kpe",        "Latn", kLeftToRight) \
  LANG("kpy",        "Cyrl", kLeftToRight) \
  LANG("kr",         "Latn", kLeftToRight) \
  LANG("krc",        "Cyrl", kLeftToRight) \
  LANG("kri",        "Latn", kLeftToRight) \
  LANG("krl",        "Latn", kLeftToRight) \
  LANG("kru",        "Deva", kLeftToRight) \
  LANG("ks",         "Arab", kRightToLeft) \
  LANG("ksb",        "Latn", kLeftToRight) \
  LANG("ksf",        "Latn", kLeftToRight) \
  LANG("ksh",        "Latn", kLeftToRight) \
  LANG("ku",         "Latn", kLeftToRight) \
  LANG("ku-LB",      "Arab", kRightToLeft) \
  LANG("kum",        "Cyrl", kLeftToRight) \
  LANG("kut",        "Latn", kLeftToRight) \
  LANG("kv",         "Cyrl", kLeftToRight) \
  LANG("kw",         "Latn", kLeftToRight) \
  LANG("ky",         "Cyrl", kLeftToRight) \
  LANG("ky-CN",      "Arab", kRightToLeft) \
  LANG("ky-TR",      "Latn", kLeftToRight) \
  LANG("kyu",        "Kali", kLeftToRight) \
  LANG("la",         "Latn", kLeftToRight) \
  LANG("lad",        "Hebr", kRightToLeft) \
  LANG("lag",        "Latn", kLeftToRight) \
  LANG("lah",        "Arab", kRightToLeft) \
  LANG("lam",        "Latn", kLeftToRight) \
  LANG("lb",         "Latn", kLeftToRight) \
  LANG("lbe",        "Cyrl", kLeftToRight) \
  LANG("lcp",        "Thai", kLeftToRight) \
  LANG("lep",        "Lepc", kLeftToRight) \
  LANG("lez",        "Cyrl", kLeftToRight) \
  LANG("lg",         "Latn", kLeftToRight) \
  LANG("li",         "Latn", kLeftToRight) \
  LANG("lif",        "Deva", kLeftToRight) \
  LANG("lis",        "Lisu", kLeftToRight) \
  LANG("lki",        "Arab", kRightToLeft) \
  LANG("lmn",        "Telu", kLeftToRight) \
  LANG("ln",         "Latn", kLeftToRight) \
  LANG("lo",         "Laoo", kLeftToRight) \
  LANG("lol",        "Latn", kLeftToRight) \
  LANG("loz",        "Latn", kLeftToRight) \
  LANG("lt",         "Latn", kLeftToRight) \
  LANG("lu",         "Latn", kLeftToRight) \
  LANG("lua",        "Latn", kLeftToRight) \
  LANG("lui",        "Latn", kLeftToRight) \
  LANG("lun",        "Latn", kLeftToRight) \
  LANG("luo",        "Latn", kLeftToRight) \
  LANG("lus",        "Beng", kLeftToRight) \
  LANG("lut",        "Latn", kLeftToRight) \
  LANG("luy",        "Latn", kLeftToRight) \
  LANG("lv",         "Latn", kLeftToRight) \
  LANG("lwl",        "Thai", kLeftToRight) \
  LANG("mad",        "Latn", kLeftToRight) \
  LANG("mag",        "Deva", kLeftToRight) \
  LANG("mai",        "Deva", kLeftToRight) \
  LANG("mak",        "Latn", kLeftToRight) \
  LANG("mak-ID",     "Bugi", kLeftToRight) \
  LANG("man",        "Latn", kLeftToRight) \
  LANG("man-GN",     "Nkoo", kRightToLeft) \
  LANG("mas",        "Latn", kLeftToRight) \
  LANG("mdf",        "Cyrl", kLeftToRight) \
  LANG("mdh",        "Latn", kLeftToRight) \
  LANG("mdr",        "Latn", kLeftToRight) \
  LANG("men",        "Latn", kLeftToRight) \
  LANG("mer",        "Latn", kLeftToRight) \
  LANG("mfe",        "Latn", kLeftToRight) \
  LANG("mg",         "Latn", kLeftToRight) \
  LANG("mgh",        "Latn", kLeftToRight) \
  LANG("mh",         "Latn", kLeftToRight) \
  LANG("mi",         "Latn", kLeftToRight) \
  LANG("mic",        "Latn", kLeftToRight) \
  LANG("min",        "Latn", kLeftToRight) \
  LANG("mk",         "Cyrl", kLeftToRight) \
  LANG("ml",         "Mlym", kLeftToRight) \
  LANG("mn",         "Cyrl", kLeftToRight) \
  LANG("mni",        "Beng", kLeftToRight) \
  LANG("mni-IN",     "Mtei", kLeftToRight) \
  LANG("mns",        "Cyrl", kLeftToRight) \
  LANG("mnw",        "Mymr", kLeftToRight) \
  LANG("moh",        "Latn", kLeftToRight) \
  LANG("mos",        "Latn", kLeftToRight) \
  LANG("mr",         "Deva", kLeftToRight) \
  LANG("ms",         "Arab", kRightToLeft) \
  LANG("ms-MY",      "Latn", kLeftToRight) \
  LANG("ms-SG",      "Latn", kLeftToRight) \
  LANG("mt",         "Latn", kLeftToRight) \
  LANG("mua",        "Latn", kLeftToRight) \
  LANG("mus",        "Latn", kLeftToRight) \
  LANG("mwl",        "Latn", kLeftToRight) \
  LANG("mwr",        "Deva", kLeftToRight) \
  LANG("my",         "Mymr", kLeftToRight) \
  LANG("myv",        "Cyrl", kLeftToRight) \
  LANG("myz",        "Mand", kRightToLeft) \
  LANG("na",         "Latn", kLeftToRight) \
  LANG("nap",        "Latn", kLeftToRight) \
  LANG("naq",        "Latn", kLeftToRight) \
  LANG("nb",         "Latn", kLeftToRight) \
  LANG("nd",         "Latn", kLeftToRight) \
  LANG("nds",        "Latn", kLeftToRight) \
  LANG("ne",         "Deva", kLeftToRight) \
  LANG("new",        "Deva", kLeftToRight) \
  LANG("ng",         "Latn", kLeftToRight) \
  LANG("nia",        "Latn", kLeftToRight) \
  LANG("niu",        "Latn", kLeftToRight) \
  LANG("nl",         "Latn", kLeftToRight) \
  LANG("nmg",        "Latn", kLeftToRight) \
  LANG("nn",         "Latn", kLeftToRight) \
  LANG("no",         "Latn", kLeftToRight) \
  LANG("nod",        "Lana", kLeftToRight) \
  LANG("nog",        "Cyrl", kLeftToRight) \
  LANG("nqo",        "Nkoo", kRightToLeft) \
  LANG("nr",         "Latn", kLeftToRight) \
  LANG("nso",        "Latn", kLeftToRight) \
  LANG("nus",        "Latn", kLeftToRight) \
  LANG("nv",         "Latn", kLeftToRight) \
  LANG("ny",         "Latn", kLeftToRight) \
  LANG("nym",        "Latn", kLeftToRight) \
  LANG("nyn",        "Latn", kLeftToRight) \
  LANG("nyo",        "Latn", kLeftToRight) \
  LANG("nzi",        "Latn", kLeftToRight) \
  LANG("oc",         "Latn", kLeftToRight) \
  LANG("oj",         "Cans", kLeftToRight) \
  LANG("om",         "Latn", kLeftToRight) \
  LANG("om-ET",      "Ethi", kLeftToRight) \
  LANG("or",         "Orya", kLeftToRight) \
  LANG("os",         "Cyrl", kLeftToRight) \
  LANG("osa",        "Latn", kLeftToRight) \
  LANG("osc",        "Ital", kLeftToRight) \
  LANG("otk",        "Orkh", kRightToLeft) \
  LANG("pa",         "Guru", kLeftToRight) \
  LANG("pa-PK",      "Arab", kRightToLeft) \
  LANG("pag",        "Latn", kLeftToRight) \
  LANG("pal",        "Phli", kRightToLeft) \
  LANG("pam",        "Latn", kLeftToRight) \
  LANG("pap",        "Latn", kLeftToRight) \
  LANG("pau",        "Latn", kLeftToRight) \
  LANG("peo",        "Xpeo", kLeftToRight) \
  LANG("phn",        "Phnx", kRightToLeft) \
  LANG("pi",         "Deva", kLeftToRight) \
  LANG("pl",         "Latn", kLeftToRight) \
  LANG("pon",        "Latn", kLeftToRight) \
  LANG("pra",        "Brah", kLeftToRight) \
  LANG("prd",        "Arab", kRightToLeft) \
  LANG("prg",        "Latn", kLeftToRight) \
  LANG("prs",        "Arab", kRightToLeft) \
  LANG("ps",         "Arab", kRightToLeft) \
  LANG("pt",         "Latn", kLeftToRight) \
  LANG("qu",         "Latn", kLeftToRight) \
  LANG("raj",        "Latn", kLeftToRight) \
  LANG("rap",        "Latn", kLeftToRight) \
  LANG("rar",        "Latn", kLeftToRight) \
  LANG("rcf",        "Latn", kLeftToRight) \
  LANG("rej",        "Latn", kLeftToRight) \
  LANG("rej-ID",     "Rjng", kLeftToRight) \
  LANG("rjs",        "Deva", kLeftToRight) \
  LANG("rkt",        "Beng", kLeftToRight) \
  LANG("rm",         "Latn", kLeftToRight) \
  LANG("rn",         "Latn", kLeftToRight) \
  LANG("ro",         "Latn", kLeftToRight) \
  LANG("ro-RS",      "Cyrl", kLeftToRight) \
  LANG("rof",        "Latn", kLeftToRight) \
  LANG("rom",        "Cyrl", kLeftToRight) \
  LANG("ru",         "Cyrl", kLeftToRight) \
  LANG("rup",        "Latn", kLeftToRight) \
  LANG("rw",         "Latn", kLeftToRight) \
  LANG("rwk",        "Latn", kLeftToRight) \
  LANG("sa",         "Deva", kLeftToRight) \
  LANG("sad",        "Latn", kLeftToRight) \
  LANG("saf",        "Latn", kLeftToRight) \
  LANG("sah",        "Cyrl", kLeftToRight) \
  LANG("sam",        "Hebr", kRightToLeft) \
  LANG("saq",        "Latn", kLeftToRight) \
  LANG("sas",        "Latn", kLeftToRight) \
  LANG("sat",        "Latn", kLeftToRight) \
  LANG("saz",        "Saur", kLeftToRight) \
  LANG("sbp",        "Latn", kLeftToRight) \
  LANG("sc",         "Latn", kLeftToRight) \
  LANG("scn",        "Latn", kLeftToRight) \
  LANG("sco",        "Latn", kLeftToRight) \
  LANG("sd",         "Arab", kRightToLeft) \
  LANG("sd-ID",      "Deva", kLeftToRight) \
  LANG("sdh",        "Arab", kRightToLeft) \
  LANG("se",         "Latn", kLeftToRight) \
  LANG("se-NO",      "Cyrl", kLeftToRight) \
  LANG("see",        "Latn", kLeftToRight) \
  LANG("seh",        "Latn", kLeftToRight) \
  LANG("sel",        "Cyrl", kLeftToRight) \
  LANG("ses",        "Latn", kLeftToRight) \
  LANG("sg",         "Latn", kLeftToRight) \
  LANG("sga",        "Latn", kLeftToRight) \
  LANG("shi",        "Tfng", kLeftToRight) \
  LANG("shn",        "Mymr", kLeftToRight) \
  LANG("si",         "Sinh", kLeftToRight) \
  LANG("sid",        "Latn", kLeftToRight) \
  LANG("sk",         "Latn", kLeftToRight) \
  LANG("sl",         "Latn", kLeftToRight) \
  LANG("sm",         "Latn", kLeftToRight) \
  LANG("sma",        "Latn", kLeftToRight) \
  LANG("smi",        "Latn", kLeftToRight) \
  LANG("smj",        "Latn", kLeftToRight) \
  LANG("smn",        "Latn", kLeftToRight) \
  LANG("sms",        "Latn", kLeftToRight) \
  LANG("sn",         "Latn", kLeftToRight) \
  LANG("snk",        "Latn", kLeftToRight) \
  LANG("so",         "Latn", kLeftToRight) \
  LANG("son",        "Latn", kLeftToRight) \
  LANG("sq",         "Latn", kLeftToRight) \
  LANG("sr",         "Latn", kLeftToRight) \
  LANG("srn",        "Latn", kLeftToRight) \
  LANG("srr",        "Latn", kLeftToRight) \
  LANG("ss",         "Latn", kLeftToRight) \
  LANG("ssy",        "Latn", kLeftToRight) \
  LANG("st",         "Latn", kLeftToRight) \
  LANG("su",         "Latn", kLeftToRight) \
  LANG("suk",        "Latn", kLeftToRight) \
  LANG("sus",        "Latn", kLeftToRight) \
  LANG("sus-GN",     "Arab", kRightToLeft) \
  LANG("sv",         "Latn", kLeftToRight) \
  LANG("sw",         "Latn", kLeftToRight) \
  LANG("swb",        "Arab", kRightToLeft) \
  LANG("swb-YT",     "Latn", kLeftToRight) \
  LANG("swc",        "Latn", kLeftToRight) \
  LANG("syl",        "Beng", kLeftToRight) \
  LANG("syl-BD",     "Sylo", kLeftToRight) \
  LANG("syr",        "Syrc", kRightToLeft) \
  LANG("ta",         "Taml", kLeftToRight) \
  LANG("tab",        "Cyrl", kLeftToRight) \
  LANG("tbw",        "Latn", kLeftToRight) \
  LANG("tcy",        "Knda", kLeftToRight) \
  LANG("tdd",        "Tale", kLeftToRight) \
  LANG("te",         "Telu", kLeftToRight) \
  LANG("tem",        "Latn", kLeftToRight) \
  LANG("teo",        "Latn", kLeftToRight) \
  LANG("ter",        "Latn", kLeftToRight) \
  LANG("tet",        "Latn", kLeftToRight) \
  LANG("tg",         "Cyrl", kLeftToRight) \
  LANG("tg-PK",      "Arab", kRightToLeft) \
  LANG("th",         "Thai", kLeftToRight) \
  LANG("ti",         "Ethi", kLeftToRight) \
  LANG("tig",        "Ethi", kLeftToRight) \
  LANG("tiv",        "Latn", kLeftToRight) \
  LANG("tk",         "Latn", kLeftToRight) \
  LANG("tkl",        "Latn", kLeftToRight) \
  LANG("tli",        "Latn", kLeftToRight) \
  LANG("tmh",        "Latn", kLeftToRight) \
  LANG("tn",         "Latn", kLeftToRight) \
  LANG("to",         "Latn", kLeftToRight) \
  LANG("tog",        "Latn", kLeftToRight) \
  LANG("tpi",        "Latn", kLeftToRight) \
  LANG("tr",         "Latn", kLeftToRight) \
  LANG("tr-ED",      "Arab", kRightToLeft) \
  LANG("tr-MK",      "Arab", kRightToLeft) \
  LANG("tru",        "Latn", kLeftToRight) \
  LANG("trv",        "Latn", kLeftToRight) \
  LANG("ts",         "Latn", kLeftToRight) \
  LANG("tsg",        "Latn", kLeftToRight) \
  LANG("tsi",        "Latn", kLeftToRight) \
  LANG("tt",         "Cyrl", kLeftToRight) \
  LANG("tts",        "Thai", kLeftToRight) \
  LANG("tum",        "Latn", kLeftToRight) \
  LANG("tut",        "Cyrl", kLeftToRight) \
  LANG("tvl",        "Latn", kLeftToRight) \
  LANG("twq",        "Latn", kLeftToRight) \
  LANG("ty",         "Latn", kLeftToRight) \
  LANG("tyv",        "Cyrl", kLeftToRight) \
  LANG("tzm",        "Latn", kLeftToRight) \
  LANG("ude",        "Cyrl", kLeftToRight) \
  LANG("udm",        "Cyrl", kLeftToRight) \
  LANG("udm-RU",     "Latn", kLeftToRight) \
  LANG("ug",         "Arab", kRightToLeft) \
  LANG("ug-KZ",      "Cyrl", kLeftToRight) \
  LANG("ug-MN",      "Cyrl", kLeftToRight) \
  LANG("uga",        "Ugar", kLeftToRight) \
  LANG("uk",         "Cyrl", kLeftToRight) \
  LANG("uli",        "Latn", kLeftToRight) \
  LANG("umb",        "Latn", kLeftToRight) \
  LANG("unr",        "Beng", kLeftToRight) \
  LANG("unr-NP",     "Deva", kLeftToRight) \
  LANG("unx",        "Beng", kLeftToRight) \
  LANG("ur",         "Arab", kRightToLeft) \
  LANG("uz",         "Latn", kLeftToRight) \
  LANG("uz-AF",      "Arab", kRightToLeft) \
  LANG("uz-CN",      "Cyrl", kLeftToRight) \
  LANG("vai",        "Vaii", kLeftToRight) \
  LANG("ve",         "Latn", kLeftToRight) \
  LANG("vi",         "Latn", kLeftToRight) \
  LANG("vi-US",      "Hani", kLeftToRight) \
  LANG("vo",         "Latn", kLeftToRight) \
  LANG("vot",        "Latn", kLeftToRight) \
  LANG("vun",        "Latn", kLeftToRight) \
  LANG("wa",         "Latn", kLeftToRight) \
  LANG("wae",        "Latn", kLeftToRight) \
  LANG("wak",        "Latn", kLeftToRight) \
  LANG("wal",        "Ethi", kLeftToRight) \
  LANG("war",        "Latn", kLeftToRight) \
  LANG("was",        "Latn", kLeftToRight) \
  LANG("wo",         "Latn", kLeftToRight) \
  LANG("xal",        "Cyrl", kLeftToRight) \
  LANG("xcr",        "Cari", kLeftToRight) \
  LANG("xh",         "Latn", kLeftToRight) \
  LANG("xog",        "Latn", kLeftToRight) \
  LANG("xpr",        "Prti", kRightToLeft) \
  LANG("xsa",        "Sarb", kRightToLeft) \
  LANG("xsr",        "Deva", kLeftToRight) \
  LANG("xum",        "Ital", kLeftToRight) \
  LANG("yao",        "Latn", kLeftToRight) \
  LANG("yap",        "Latn", kLeftToRight) \
  LANG("yav",        "Latn", kLeftToRight) \
  LANG("yi",         "Hebr", kRightToLeft) \
  LANG("yo",         "Latn", kLeftToRight) \
  LANG("yrk",        "Cyrl", kLeftToRight) \
  LANG("yue",        "Hans", kLeftToRight) \
  LANG("za",         "Latn", kLeftToRight) \
  LANG("za-CN",      "Hans", kLeftToRight) \
  LANG("zap",        "Latn", kLeftToRight) \
  LANG("zen",        "Tfng", kLeftToRight) \
  LANG("zh",         "Hant", kLeftToRight) \
  LANG("zh-CN",      "Hans", kLeftToRight) \
  LANG("zh-HK",      "Hans", kLeftToRight) \
  LANG("zh-MN",      "Hans", kLeftToRight) \
  LANG("zh-MO",      "Hans", kLeftToRight) \
  LANG("zh-SG",      "Hans", kLeftToRight) \
  LANG("zu",         "Latn", kLeftToRight) \
  LANG("zun",        "Latn", kLeftToRight) \
  LANG("zza",        "Arab", kRightToLeft)

namespace redux {

struct LocaleData {
  const char* language_iso_639 = "";
  const char* script_iso_15924 = "";
  TextDirection default_direction;
};

#define LANG(_lang, _script, _dir) \
    { _lang, _script, TextDirection::_dir},
static constexpr LocaleData gLocalData[] = {
  REDUX_TEXT_LANGUAGE_TUPLE
};
#undef LANG

static constexpr size_t kNumLocales =
    (sizeof(gLocalData) / sizeof(gLocalData[0]));

static const LocaleData* FindLocaleData(const char* language_iso_639) {
  // TopToBottom currently unsupported. Once supported, add this to the tuple
  // above.
  // LANG("mn-CN",      "Mong", kTopToBottom)
  // LANG("mnc",        "Mong", kTopToBottom)
  CHECK_NE(strcmp(language_iso_639, "mn-CN"), 0);
  CHECK_NE(strcmp(language_iso_639, "mnc"), 0);

  auto begin = gLocalData;
  auto end = begin + kNumLocales;
  auto iter =
      std::lower_bound(begin, end, language_iso_639,
                       [](const LocaleData& locale, const char* key) {
                         return strcmp(locale.language_iso_639, key) < 0;
                       });
  return iter != end ? iter : nullptr;
}

const char* GetTextScriptIso15924(const char* language_iso_639) {
  auto locale = FindLocaleData(language_iso_639);
  CHECK(locale);
  return locale->script_iso_15924;
}

TextDirection GetDefaultTextDirection(const char* language_iso_639) {
  auto locale = FindLocaleData(language_iso_639);
  CHECK(locale);
  return locale->default_direction;
}

}  // namespace redux
