#  -*- coding: utf-8 -*-
#@PydevCodeAnalysisIgnore

import sys, re, codecs

if len(sys.argv) < 5:
	print "x2po pot source locale translator project"
	sys.exit(0)

pot = sys.argv[1]
source = sys.argv[2]
locale = sys.argv[3]
translator = sys.argv[4]
project = sys.argv[5]

langs = {
	 "aa": "Afar",
	 "ab": "Abkhazian",
	 "ace": "Achinese",
	 "ad": "Adangme",
	 "ae": "Avestan",
	 "af": "Afrikaans",
	 "ak": "Akan",
	 "am": "Amharic",
	 "an": "Aragonese",
	 "ang": "Old English",
	 "ar": "Arabic",
	 "as": "Assamese",
	 "av": "Avaric",
	 "awa": "Awadhi",
	 "ay": "Aymara",
	 "az": "Azerbaijani",
	 "ba": "Bashkir",
	 "bad": "Banda",
	 "bal": "Baluchi",
	 "ban": "Balinese",
	 "be": "Belarusian",
	 "bem": "Bemba",
	 "bg": "Bulgarian",
	 "bh": "Bihari",
	 "bho": "Bhojpuri",
	 "bi": "Bislama",
	 "bik": "Bikol",
	 "bin": "Bini",
	 "bm": "Bambara",
	 "bn": "Bengali",
	 "bo": "Tibetan",
	 "br": "Breton",
	 "bs": "Bosnian",
	 "btk": "Batak",
	 "bug": "Buginese",
	 "ca": "Catalan",
	 "ce": "Chechen",
	 "ceb": "Cebuano",
	 "ch": "Chamorro",
	 "co": "Corsican",
	 "cr": "Cree",
	 "cs": "Czech",
	 "csb": "Kashubian",
	 "cu": "Church Slavic",
	 "cv": "Chuvash",
	 "cy": "Welsh",
	 "da": "Danish",
	 "de": "German",
	 "din": "Dinka",
	 "doi": "Dogri",
	 "dv": "Divehi",
	 "dz": "Dzongkha",
	 "ee": "Ewe",
	 "el": "Greek",
	 "en": "English",
	 "eo": "Esperanto",
	 "es": "Spanish",
	 "et": "Estonian",
	 "eu": "Basque",
	 "fa": "Persian",
	 "ff": "Fulah",
	 "fi": "Finnish",
	 "fil": "Filipino",
	 "fj": "Fijian",
	 "fo": "Faroese",
	 "fon": "Fon",
	 "fr": "French",
	 "fy": "Western Frisian",
	 "ga": "Irish",
	 "gd": "Scots",
	 "gl": "Galician",
	 "gn": "Guarani",
	 "gon": "Gondi",
	 "gsw": "Swiss German",
	 "gu": "Gujarati",
	 "gv": "Manx",
	 "ha": "Hausa",
	 "he": "Hebrew",
	 "hi": "Hindi",
	 "hil": "Hiligaynon",
	 "hmn": "Hmong",
	 "ho": "Hiri Motu",
	 "hr": "Croatian",
	 "ht": "Haitian",
	 "hu": "Hungarian",
	 "hy": "Armenian",
	 "hz": "Herero",
	 "ia": "Interlingua",
	 "id": "Indonesian",
	 "ie": "Interlingue",
	 "ig": "Igbo",
	 "ii": "Sichuan Yi",
	 "ik": "Inupiak",
	 "ilo": "Iloko",
	 "is": "Icelandic",
	 "it": "Italian",
	 "iu": "Inuktitut",
	 "ja": "Japanese",
	 "jab": "Hyam",
	 "jv": "Javanese",
	 "ka": "Georgian",
	 "kab": "Kabyle",
	 "kaj": "Jju",
	 "kam": "Kamba",
	 "kbd": "Kabardian",
	 "kcg": "Tyap",
	 "kdm": "Kagoma",
	 "kg": "Kongo",
	 "ki": "Kikuyu",
	 "kj": "Kuanyama",
	 "kk": "Kazakh",
	 "kl": "Kalaallisut",
	 "km": "Khmer",
	 "kmb": "Kimbundu",
	 "kn": "Kannada",
	 "ko": "Korean",
	 "kr": "Kanuri",
	 "kru": "Kurukh",
	 "ks": "Kashmiri",
	 "ku": "Kurdish",
	 "kv": "Komi",
	 "kw": "Cornish",
	 "ky": "Kirghiz",
	 "kok": "Konkani",
	 "la": "Latin",
	 "lb": "Letzeburgesch",
	 "lg": "Ganda",
	 "li": "Limburgish",
	 "ln": "Lingala",
	 "lo": "Laotian",
	 "lt": "Lithuanian",
	 "lu": "Luba-Katanga",
	 "lua": "Luba-Lulua",
	 "luo": "Luo",
	 "lv": "Latvian",
	 "mad": "Madurese",
	 "mag": "Magahi",
	 "mai": "Maithili",
	 "mak": "Makasar",
	 "man": "Mandingo",
	 "men": "Mende",
	 "mg": "Malagasy",
	 "mh": "Marshallese",
	 "mi": "Maori",
	 "min": "Minangkabau",
	 "mk": "Macedonian",
	 "ml": "Malayalam",
	 "mn": "Mongolian",
	 "mni": "Manipuri",
	 "mo": "Moldavian",
	 "mos": "Mossi",
	 "mr": "Marathi",
	 "ms": "Malay",
	 "mt": "Maltese",
	 "mwr": "Marwari",
	 "my": "Burmese",
	 "myn": "Mayan",
	 "na": "Nauru",
	 "nap": "Neapolitan",
	 "nah": "Nahuatl",
	 "nb": "Norwegian Bokmal",
	 "nd": "North Ndebele",
	 "nds": "Low Saxon",
	 "ne": "Nepali",
	 "ng": "Ndonga",
	 "nl": "Dutch",
	 "nn": "Norwegian Nynorsk",
	 "no": "Norwegian",
	 "nr": "South Ndebele",
	 "nso": "Northern Sotho",
	 "nv": "Navajo",
	 "ny": "Nyanja",
	 "nym": "Nyamwezi",
	 "nyn": "Nyankole",
	 "oc": "Occitan",
	 "oj": "Ojibwa",
	 "om": "(Afan) Oromo",
	 "or": "Oriya",
	 "os": "Ossetian",
	 "pa": "Punjabi",
	 "pag": "Pangasinan",
	 "pam": "Pampanga",
	 "pbb": "Páez",
	 "pi": "Pali",
	 "pl": "Polish",
	 "ps": "Pashto",
	 "pt": "Portuguese",
	 "qu": "Quechua",
	 "raj": "Rajasthani",
	 "rm": "Rhaeto-Roman",
	 "rn": "Kirundi",
	 "ro": "Romanian",
	 "ru": "Russian",
	 "rw": "Kinyarwanda",
	 "sa": "Sanskrit",
	 "sas": "Sasak",
	 "sat": "Santali",
	 "sc": "Sardinian",
	 "scn": "Sicilian",
	 "sd": "Sindhi",
	 "se": "Northern Sami",
	 "sg": "Sango",
	 "shn": "Shan",
	 "si": "Sinhala",
	 "sid": "Sidamo",
	 "sk": "Slovak",
	 "sl": "Slovenian",
	 "sm": "Samoan",
	 "sn": "Shona",
	 "so": "Somali",
	 "sq": "Albanian",
	 "sr": "Serbian",
	 "srr": "Serer",
	 "ss": "Siswati",
	 "st": "Sesotho",
	 "su": "Sundanese",
	 "suk": "Sukuma",
	 "sus": "Susu",
	 "sv": "Swedish",
	 "sw": "Swahili",
	 "ta": "Tamil",
	 "te": "Telugu",
	 "tem": "Timne",
	 "tet": "Tetum",
	 "tg": "Tajik",
	 "th": "Thai",
	 "ti": "Tigrinya",
	 "tiv": "Tiv",
	 "tk": "Turkmen",
	 "tl": "Tagalog",
	 "tn": "Setswana",
	 "to": "Tonga",
	 "tr": "Turkish",
	 "ts": "Tsonga",
	 "tt": "Tatar",
	 "tum": "Tumbuka",
	 "tw": "Twi",
	 "ty": "Tahitian",
	 "ug": "Uighur",
	 "uk": "Ukrainian",
	 "umb": "Umbundu",
	 "ur": "Urdu",
	 "uz": "Uzbek",
	 "ve": "Venda",
	 "vi": "Vietnamese",
	 "vo": "Volapuk",
	 "wal": "Walamo",
	 "war": "Waray",
	 "wen": "Sorbian",
	 "wo": "Wolof",
	 "xh": "Xhosa",
	 "yao": "Yao",
	 "yi": "Yiddish",
	 "yo": "Yoruba",
	 "za": "Zhuang",
	 "zh": "Chinese",
	 "zu": "Zulu",
	 "zap": "Zapotec" 
}
plurals = {
	 "ja":		  "nplurals=1; plural=0;" ,
	 "ko":			"nplurals=1; plural=0;" ,
	 "vi":		"nplurals=1; plural=0;" ,
	 "tr":		   "nplurals=1; plural=0;" ,
	 "da":			"nplurals=2; plural=(n != 1);" ,
	 "nl":			 "nplurals=2; plural=(n != 1);" ,
	 "en":		   "nplurals=2; plural=(n != 1);" ,
	 "fo":		   "nplurals=2; plural=(n != 1);" ,
	 "de":			"nplurals=2; plural=(n != 1);" ,
	 "nb":  "nplurals=2; plural=(n != 1);" ,
	 "no":		 "nplurals=2; plural=(n != 1);" ,
	 "nn": "nplurals=2; plural=(n != 1);" ,
	 "sv":		   "nplurals=2; plural=(n != 1);" ,
	 "et":		  "nplurals=2; plural=(n != 1);" ,
	 "fi":		   "nplurals=2; plural=(n != 1);" ,
	 "el":			 "nplurals=2; plural=(n != 1);" ,
	 "he":			"nplurals=2; plural=(n != 1);" ,
	 "it":		   "nplurals=2; plural=(n != 1);" ,
	 "pt":		"nplurals=2; plural=(n != 1);" ,
	 "es":		   "nplurals=2; plural=(n != 1);" ,
	 "eo":		 "nplurals=2; plural=(n != 1);" ,
	 "hu":		 "nplurals=2; plural=(n != 1);" ,
	 "fr":			"nplurals=2; plural=(n > 1);" ,
	 "pt_BR":	  "nplurals=2; plural=(n > 1);" ,
	 "lv":		   "nplurals=3; plural=(n%10==1 && n%100!=11 ? 0 : n != 0 ? 1 : 2);" ,
	 "ga":			 "nplurals=3; plural=n==1 ? 0 : n==2 ? 1 : 2;" ,
	 "ro":		  "nplurals=3; plural=n==1 ? 0 : (n==0 || (n%100 > 0 && n%100 < 20)) ? 1 : 2;" ,
	 "lt":		"nplurals=3; plural=(n%10==1 && n%100!=11 ? 0 : n%10>=2 && (n%100<10 || n%100>=20) ? 1 : 2);" ,
	 "hr":		  "nplurals=3; plural=(n%10==1 && n%100!=11 ? 0 : n%10>=2 && n%10<=4 && (n%100<10 || n%100>=20) ? 1 : 2);" ,
	 "sr":		   "nplurals=3; plural=(n%10==1 && n%100!=11 ? 0 : n%10>=2 && n%10<=4 && (n%100<10 || n%100>=20) ? 1 : 2);" ,
	 "ru":		   "nplurals=3; plural=(n%10==1 && n%100!=11 ? 0 : n%10>=2 && n%10<=4 && (n%100<10 || n%100>=20) ? 1 : 2);" ,
	 "uk":		 "nplurals=3; plural=(n%10==1 && n%100!=11 ? 0 : n%10>=2 && n%10<=4 && (n%100<10 || n%100>=20) ? 1 : 2);" ,
	 "sk":			"nplurals=3; plural=(n==1) ? 0 : (n>=2 && n<=4) ? 1 : 2;" ,
	 "cs":			 "nplurals=3; plural=(n==1) ? 0 : (n>=2 && n<=4) ? 1 : 2;" ,
	 "pl":			"nplurals=3; plural=(n==1 ? 0 : n%10>=2 && n%10<=4 && (n%100<10 || n%100>=20) ? 1 : 2);" ,
	 "sl":		 "nplurals=4; plural=(n%100==1 ? 0 : n%100==2 ? 1 : n%100==3 || n%100==4 ? 2 : 3);" 	
		
}

ids = []
for line in codecs.open(pot, "r", "utf-8"):
	m = re.search(ur'^msgid "([^"]+)', line)
	if m:
		ids += [m.group(1)]
		
oldmap = { }
sre = re.compile(ur'<String Name="([^"]+)">([^<]+)<')

for line in codecs.open(u"Example.xml", "r", "utf-8"):
	m = sre.search(line)
	if m:
		oldmap[m.group(1)] = m.group(2)

newmap = { }
for line in codecs.open(source, "r", "utf-8"):
	m = sre.search(line)
	if m:
		s = oldmap.get(m.group(1))
		if s and s in ids:
			newmap[s] = m.group(2).replace('"', '\\"')

out = codecs.open(project + "/po/template-" + locale + ".po", "w", "utf-8")
plural = plurals.get(locale, "plurals=2; plural=(n != 1);")
language = langs.get(locale, "XXX");

out.write(r"""
# LANGUAGE translations for the DC++ package.
# Copyright (C) 2008 The translators
# This file is distributed under the same license as the DC++ package.
#  <TRANSLATOR>, 2008.
#
msgid ""
msgstr ""
"Project-Id-Version: PROJECT\n"
"Report-Msgid-Bugs-To: dcplusplus-devel@lists.sourceforge.net\n"
"Last-Translator:  <TRANSLATOR>\n"
"MIME-Version: 1.0\n"
"Content-Type: text/plain; charset=UTF-8\n"
"Content-Transfer-Encoding: 8bit\n"
"Plural-Forms: PLURAL\n"

""".replace("TRANSLATOR", translator).replace("PROJECT", project).replace("PLURAL", plural).replace("LANGUAGE", language)
)

for k, v in newmap.items():
	out.write(u'msgid "' + k + u'"\n')
	out.write(u'msgstr "' + v + u'"\n')
	out.write(u'\n')

out.close()
