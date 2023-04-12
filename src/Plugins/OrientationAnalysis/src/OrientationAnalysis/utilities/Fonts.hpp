#pragma once
#include <vector>
namespace fonts
{

// Valid TTF file, cmap table has types 12 and 4 subtables.
std::vector<unsigned char> font_a;
char const font_a_base64[] = "AAEAAAALAIAAAwAwT1MvMmisck8AAAE4AAAAYGNtYXAXewGCAAAB3AAAAUJjdnQgAEQFEQAA"
                             "AyAAAAAEZ2x5ZjCUlAIAAANMAAAGhmhlYWQe1bIjAAAAvAAAADZoaGVhDf8FBAAAAPQAAAAk"
                             "aG10eDmaBAMAAAGYAAAARGxvY2ERbxMOAAADJAAAAChtYXhwAHUAtwAAARgAAAAgbmFtZVZp"
                             "NvsAAAnUAAAA23Bvc3T/aQBmAAAKsAAAACAAAQAAAAEAAEPW4v5fDzz1AB0IAAAAAADcB1gv"
                             "AAAAANwUDpf/+f5tB5AH8wAAAAgAAgAAAAAAAAABAAAFu/+6ALgIAP/5/ToHkAABAAAAAAAA"
                             "AAAAAAAAAAAADwABAAAAEwBAABAAcAAIAAIAAAABAAEAAABAAAMACAABAAQD/wGQAAUAAAUz"
                             "BZkAAAEeBTMFmQAAA9cAZgISAAACAAUDAAAAAAAAAAAAQwIAAAAEAAAAAAAAAFBmRWQAgAAg"
                             "//8GQP5AALgFuwBGAAAAAQAAAAADmwW3AAAAIAABAuwARAQAAAAFogAiBikAVwK0ABQDqAA8"
                             "BGwANALYAE8CsQA8A8j/+QPI//kCtAAUAAABBQgAAAADhABkAGQAZABkAGQAAAACAAMAAQAA"
                             "ABQAAwAKAAAAigAEAHYAAAAWABAAAwAGACAAKgBJAGEAbgB0AHYAeQDNAwH//wAAACAAKgBD"
                             "AGEAbgBzAHYAeQDNAwH////h/9gAAP+k/5j/lP+T/5H/Pv0LAAEAAAAAABIAAAAAAAAAAAAA"
                             "AAAAAAAAAAMAEgAOAA8AEAARAAQADAAAAAAAuAAAAAAAAAAOAAAAIAAAACAAAAABAAAAKgAA"
                             "ACoAAAACAAAAQwAAAEMAAAADAAAARAAAAEQAAAASAAAARQAAAEgAAAAOAAAASQAAAEkAAAAE"
                             "AAAAYQAAAGEAAAAFAAAAbgAAAG4AAAAGAAAAcwAAAHQAAAAHAAAAdgAAAHYAAAAJAAAAeQAA"
                             "AHkAAAAKAAAAzQAAAM0AAAALAAADAQAAAwEAAAAMABD//QAQ//0AAAANAAAARAURAAAAFgAW"
                             "AFQAkwDSAR8BbQGtAeoCIAJhAm8CjQMRAx0DJQMtAzUDQwACAEQAAAJkBVUAAwAHAAOxAQAz"
                             "ESERJSERIUQCIP4kAZj+aAVV+qtEBM0A//8AIgBYBYEFpxCnAAwFogRQ0sAtPtLA0sAQpwAM"
                             "AX4F2NLA0sAtPtLAEKcADAACAawtPtLALT4tPhCnAAwEJAAoLT4tPtLALT4QpwAM/+oEDQAA"
                             "wABAAAAAEKcADAW+Ae4AAEAAwAAAABAvAAwD3gXswAAQBwAMAcIADAABAFf/4gW7BbsAIwAA"
                             "ExA3NiEyBRYVFAcGJwIhIAMGFRQXFiEgEzYXFgcGBwQhIAEmV7jWAY6lATUPEhIGoP7k/t+6"
                             "jZamAWkBM50JGBcCGBv+9/7M/rX+6pECxAEo1vmQB90JAwILATv++8XE9tvyAToSBQQSzRGf"
                             "ARGOAAABABT/+gJ8BbQAIwAAMyInJjc2NxI3NgMmJyY3NjMkJRYXFgcGBwIXFhMWFxYXFgcG"
                             "NxcBARefBA0BARUJoBUBARUBIgEIGwEBG7YEDAICDAO4HQIBH/8MCgg+aQFPvqYBXJUXAxUS"
                             "BAYBFw0HNYX+u7y1/qh1HwUZDQEGAAACADz/7wN5A5EACAAuAAA3Fjc2JyYHDgI+AycmJyYH"
                             "BhcWBwYnJjc2MzIDAhcWNzY3NgcGBwYnBicmJ+IDjJYDATJLpqRFkImHAgJAKE5zBAVyIhAJ"
                             "HbLN6hcUBAVNQA4qDCqZZVKQbLYEw4UND9pgDxNUO2YoLC6NfjgiBAZBOCMKLhwcq/7J/vRg"
                             "jxcTAwoifQUDdXUBAq4AAQA0//8ETgO2ADMAADMiNTQzMgMmNzYnNjMyBwYHJDc2ExIXFjcy"
                             "FRQjMCEiNTQ3NicwAyYHBgcwAwI3NhcWJyBQHDBkDQYBAUueQDoSFQIBBovUBwkDAmcSFf6m"
                             "JSFHAgUB2XpbCQ5qLQMDDv7SHhUBlbxgTCFlLzc1dAQH/ur+oo9oARoWIxgHEUQB2MoJBUP+"
                             "cv7cBQIcIgEAAQRP/+4GiQObACUAACUmNzYzMhcWNzY3NicmNzY3NhcWBwYnJicmBwYHBhcW"
                             "FxYHBiUmBFUGCAMVFAxWbJcLBqzgGiv3bWQPBgEXFA5lPGEpHJlKTFQFCf7c1zM6VBwcug4T"
                             "o01ph5P6BAI4EogUBAQYoAIDkmRmMkNJg+UBAQABADz/7AKEBBEAIwAAEyYnJjc2NzYXFgcG"
                             "FxY3FhUUBwYnJgcCFxYXFjcGJyYTEjU0aCIGBBxcQhUKIAMIVD+VMjKMTk8BCAgJoVVJOc3z"
                             "ERQDLgUXEBZKQhUECyBQAgEHCi41AwcBAVH+u4mnAQEnlAQFAQEBNKpSAAH/+f+6A7QDjAAe"
                             "AAAlJgEmJwUyFRQHBhUUEzYTNicmJzQ3NjcGBwAHBgciAbYX/tMRaAFkHh494U93Bz4sASik"
                             "hV8Y/uEJDR4kDoACfSRdAhYSCxZAJv4/LwHaGRIMGhABAgU9Rv1/VHkBAAH/+f5tA7QDjAAm"
                             "AAAlNAEmJwUyFRQHBhUUEzYTNicmJzQ3NjcGBwIHAgcGIyY1Njc2NzYBqv7IEWgBZB4ePeFF"
                             "gQc+LAEopIVfGOJGngEYOFgBWSAGWixJApYkXQIWEgsWQCb+PygB4RoRDBoQAQIFPUb927D+"
                             "cQM1AVAZGgkNy///ABT/+gLXB/MQZwAMABEC1T/4QAASBgAEAAAAAQEFAyMCxgUeAA0AAAE2"
                             "EzY3NhcWBwYHBicmARAwqBoOWkoSHsKSFBwfA0prASUtAxQYBSf+ohcHBwAAEAAA/nAHkAYA"
                             "AAMABwALAA8AEwAXABsAHwAjACcAKwAvADMANwA7AD8AABAQIBAAECARABAhEAAQIRESESAQ"
                             "ABEgEQARIRAAESERExAgEAEQIBEBECEQARAhERMRIBABESARAREhEAERIREBkP5wAZD+cAGQ"
                             "/nABkHABkP5wAZD+cAGQ/nABkHABkP5wAZD+cAGQ/nABkHABkP5wAZD+cAGQ/nABkP5wAZD+"
                             "cAIAAZD+cAIAAZD+cAIAAZD+cPoAAZD+cAIAAZD+cAIAAZD+cAIAAZD+cPoAAZD+cAIAAZD+"
                             "cAIAAZD+cAIAAZD+cPoAAZD+cAIAAZD+cAIAAZD+cAIAAZD+cAD//wBkADIDIAWqECcAEgAA"
                             "/qIABAASAAP//wBkAZADIARMEAYAEgAA//8AZAGQAyAETBAGABIAAP//AGQBkAMgBEwQBgAS"
                             "AAAAAQBkAZADIARMAAMAABIgECBkArz9RARM/UQAAAAAAAAMAJYAAQAAAAAAAQAFAAAAAQAA"
                             "AAAAAgAHAAUAAQAAAAAAAwAFAAAAAQAAAAAABAAFAAAAAQAAAAAABQALAAwAAQAAAAAABgAF"
                             "AAAAAwABBAkAAQAKABcAAwABBAkAAgAOACEAAwABBAkAAwAKABcAAwABBAkABAAKABcAAwAB"
                             "BAkABQAWAC8AAwABBAkABgAKABdGb250QVJlZ3VsYXJWZXJzaW9uIDEuMABGAG8AbgB0AEEA"
                             "UgBlAGcAdQBsAGEAcgBWAGUAcgBzAGkAbwBuACAAMQAuADAAAAMAAAAAAAD/ZgBmAAAAAAAA"
                             "AAAAAAAAAAAAAAAAAAA=";

// Valid TTF file, cmap table has type 4 subtable only and loca table is long.
std::vector<unsigned char> font_b;
char const font_b_base64[] = "AAEAAAALAIAAAwAwT1MvMmirdVEAAAE4AAAAYGNtYXAHhQC5AAAB3AAAAIJjdnQgAEQFEQAA"
                             "AmAAAAAEZ2x5ZjCUlAIAAAK0AAAGhmhlYWQe1LMzAAAAvAAAADZoaGVhDf8FBAAAAPQAAAAk"
                             "aG10eDmaBAMAAAGYAAAARGxvY2EAAEj6AAACZAAAAFBtYXhwAHUAtwAAARgAAAAgbmFtZVZp"
                             "OPsAAAk8AAAA23Bvc3T/aQBmAAAKGAAAACAAAAEAAAEAAIakcHRfDzz1AB0IAAAAAADcB1gv"
                             "AAAAANwUDqb/+f5tB5AH8wAAAAgAAgABAAAAAAABAAAFu/+6ALgIAP/5/ToHkAABAAAAAAAA"
                             "AAAAAAAAAAAADwABAAAAEwBAABAAcAAIAAIAAAABAAEAAABAAAMACAABAAQD/wGQAAUAAAUz"
                             "BZkAAAEeBTMFmQAAA9cAZgISAAACAAUDAAAAAAAAAAAAQwIAAAAEAAAAAAAAAFBmRWQAgAAg"
                             "AwEGQP5AALgFuwBGAAAAAQAAAAADmwW3AAAAIAABAuwARAQAAAAFogAiBikAVwK0ABQDqAA8"
                             "BGwANALYAE8CsQA8A8j/+QPI//kCtAAUAAABBQgAAAADhABkAGQAZABkAGQAAAABAAMAAQAA"
                             "AAwABAB2AAAAFgAQAAMABgAgACoASQBhAG4AdAB2AHkAzQMB//8AAAAgACoAQwBhAG4AcwB2"
                             "AHkAzQMB////4f/YAAD/pP+Y/5T/k/+R/z79CwABAAAAAAASAAAAAAAAAAAAAAAAAAAAAAAD"
                             "ABIADgAPABAAEQAEAAAARAURAAAAAAAAACwAAAAsAAAAqAAAASYAAAGkAAACPgAAAtoAAANa"
                             "AAAD1AAABEAAAATCAAAE3gAABRoAAAYiAAAGOgAABkoAAAZaAAAGagAABoYAAgBEAAACZAVV"
                             "AAMABwADsQEAMxEhESUhESFEAiD+JAGY/mgFVfqrRATNAP//ACIAWAWBBacQpwAMBaIEUNLA"
                             "LT7SwNLAEKcADAF+BdjSwNLALT7SwBCnAAwAAgGsLT7SwC0+LT4QpwAMBCQAKC0+LT7SwC0+"
                             "EKcADP/qBA0AAMAAQAAAABCnAAwFvgHuAABAAMAAAAAQLwAMA94F7MAAEAcADAHCAAwAAQBX"
                             "/+IFuwW7ACMAABMQNzYhMgUWFRQHBicCISADBhUUFxYhIBM2FxYHBgcEISABJle41gGOpQE1"
                             "DxISBqD+5P7fuo2WpgFpATOdCRgXAhgb/vf+zP61/uqRAsQBKNb5kAfdCQMCCwE7/vvFxPbb"
                             "8gE6EgUEEs0RnwERjgAAAQAU//oCfAW0ACMAADMiJyY3NjcSNzYDJicmNzYzJCUWFxYHBgcC"
                             "FxYTFhcWFxYHBjcXAQEXnwQNAQEVCaAVAQEVASIBCBsBARu2BAwCAgwDuB0CAR//DAoIPmkB"
                             "T76mAVyVFwMVEgQGARcNBzWF/ru8tf6odR8FGQ0BBgAAAgA8/+8DeQORAAgALgAANxY3Nicm"
                             "Bw4CPgMnJicmBwYXFgcGJyY3NjMyAwIXFjc2NzYHBgcGJwYnJifiA4yWAwEyS6akRZCJhwIC"
                             "QChOcwQFciIQCR2yzeoXFAQFTUAOKgwqmWVSkGy2BMOFDQ/aYA8TVDtmKCwujX44IgQGQTgj"
                             "Ci4cHKv+yf70YI8XEwMKIn0FA3V1AQKuAAEANP//BE4DtgAzAAAzIjU0MzIDJjc2JzYzMgcG"
                             "ByQ3NhMSFxY3MhUUIzAhIjU0NzYnMAMmBwYHMAMCNzYXFicgUBwwZA0GAQFLnkA6EhUCAQaL"
                             "1AcJAwJnEhX+piUhRwIFAdl6WwkOai0DAw7+0h4VAZW8YEwhZS83NXQEB/7q/qKPaAEaFiMY"
                             "BxFEAdjKCQVD/nL+3AUCHCIBAAEET//uBokDmwAlAAAlJjc2MzIXFjc2NzYnJjc2NzYXFgcG"
                             "JyYnJgcGBwYXFhcWBwYlJgRVBggDFRQMVmyXCwas4Bor921kDwYBFxQOZTxhKRyZSkxUBQn+"
                             "3NczOlQcHLoOE6NNaYeT+gQCOBKIFAQEGKACA5JkZjJDSYPlAQEAAQA8/+wChAQRACMAABMm"
                             "JyY3Njc2FxYHBhcWNxYVFAcGJyYHAhcWFxY3BicmExI1NGgiBgQcXEIVCiADCFQ/lTIyjE5P"
                             "AQgICaFVSTnN8xEUAy4FFxAWSkIVBAsgUAIBBwouNQMHAQFR/ruJpwEBJ5QEBQEBATSqUgAB"
                             "//n/ugO0A4wAHgAAJSYBJicFMhUUBwYVFBM2EzYnJic0NzY3BgcABwYHIgG2F/7TEWgBZB4e"
                             "PeFPdwc+LAEopIVfGP7hCQ0eJA6AAn0kXQIWEgsWQCb+Py8B2hkSDBoQAQIFPUb9f1R5AQAB"
                             "//n+bQO0A4wAJgAAJTQBJicFMhUUBwYVFBM2EzYnJic0NzY3BgcCBwIHBiMmNTY3Njc2Aar+"
                             "yBFoAWQeHj3hRYEHPiwBKKSFXxjiRp4BGDhYAVkgBlosSQKWJF0CFhILFkAm/j8oAeEaEQwa"
                             "EAECBT1G/duw/nEDNQFQGRoJDcv//wAU//oC1wfzEGcADAARAtU/+EAAEgYABAAAAAEBBQMj"
                             "AsYFHgANAAABNhM2NzYXFgcGBwYnJgEQMKgaDlpKEh7CkhQcHwNKawElLQMUGAUn/qIXBwcA"
                             "ABAAAP5wB5AGAAADAAcACwAPABMAFwAbAB8AIwAnACsALwAzADcAOwA/AAAQECAQABAgEQAQ"
                             "IRAAECEREhEgEAARIBEAESEQABEhERMQIBABECARARAhEAEQIRETESAQAREgEQERIRABESER"
                             "AZD+cAGQ/nABkP5wAZBwAZD+cAGQ/nABkP5wAZBwAZD+cAGQ/nABkP5wAZBwAZD+cAGQ/nAB"
                             "kP5wAZD+cAGQ/nACAAGQ/nACAAGQ/nACAAGQ/nD6AAGQ/nACAAGQ/nACAAGQ/nACAAGQ/nD6"
                             "AAGQ/nACAAGQ/nACAAGQ/nACAAGQ/nD6AAGQ/nACAAGQ/nACAAGQ/nACAAGQ/nAA//8AZAAy"
                             "AyAFqhAnABIAAP6iAAQAEgAD//8AZAGQAyAETBAGABIAAP//AGQBkAMgBEwQBgASAAD//wBk"
                             "AZADIARMEAYAEgAAAAEAZAGQAyAETAADAAASIBAgZAK8/UQETP1EAAAAAAAADACWAAEAAAAA"
                             "AAEABQAAAAEAAAAAAAIABwAFAAEAAAAAAAMABQAAAAEAAAAAAAQABQAAAAEAAAAAAAUACwAM"
                             "AAEAAAAAAAYABQAAAAMAAQQJAAEACgAXAAMAAQQJAAIADgAhAAMAAQQJAAMACgAXAAMAAQQJ"
                             "AAQACgAXAAMAAQQJAAUAFgAvAAMAAQQJAAYACgAXRm9udEJSZWd1bGFyVmVyc2lvbiAxLjAA"
                             "RgBvAG4AdABCAFIAZQBnAHUAbABhAHIAVgBlAHIAcwBpAG8AbgAgADEALgAwAAADAAAAAAAA"
                             "/2YAZgAAAAAAAAAAAAAAAAAAAAAAAAAA";

// Valid TTF file, cmap table has type 0 subtable only.
std::vector<unsigned char> font_c;
char const font_c_base64[] = "AAEAAAALAIAAAwAwT1MvMmisck8AAAE4AAAAYGNtYXAgGy9CAAAB3AAAARJjdnQgAEQFEQAA"
                             "AvAAAAAEZ2x5ZjCUlAIAAAMcAAAGhmhlYWQe1bJmAAAAvAAAADZoaGVhDf8FBAAAAPQAAAAk"
                             "aG10eDmaBAMAAAGYAAAARGxvY2ERbxMOAAAC9AAAAChtYXhwAHUAtwAAARgAAAAgbmFtZVZp"
                             "OvsAAAmkAAAA23Bvc3T/aQBmAAAKgAAAACAAAQAAAAEAADKWgBhfDzz1AB0IAAAAAADcB1gv"
                             "AAAAANwUDtr/+f5tB5AH8wAAAAgAAgAAAAAAAAABAAAFu/+6ALgIAP/5/ToHkAABAAAAAAAA"
                             "AAAAAAAAAAAADwABAAAAEwBAABAAcAAIAAIAAAABAAEAAABAAAMACAABAAQD/wGQAAUAAAUz"
                             "BZkAAAEeBTMFmQAAA9cAZgISAAACAAUDAAAAAAAAAAAAQwIAAAAEAAAAAAAAAFBmRWQAgAAg"
                             "//8GQP5AALgFuwBGAAAAAQAAAAADmwW3AAAAIAABAuwARAQAAAAFogAiBikAVwK0ABQDqAA8"
                             "BGwANALYAE8CsQA8A8j/+QPI//kCtAAUAAABBQgAAAADhABkAGQAZABkAGQAAAABAAEAAAAA"
                             "AAwAAAEGAAABAAAAAAAAAAEBAAAAAQAAAAAAAAAAAAAAAAAAAAEAAAEAAAAAAAAAAAACAAAA"
                             "AAAAAAAAAAAAAAAAAAAAAAAAAAAAAxIODxARBAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAABQAA"
                             "AAAAAAAAAAAAAAYAAAAABwgACQAACgAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAA"
                             "AAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAACwAA"
                             "AAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAARAUR"
                             "AAAAFgAWAFQAkwDSAR8BbQGtAeoCIAJhAm8CjQMRAx0DJQMtAzUDQwACAEQAAAJkBVUAAwAH"
                             "AAOxAQAzESERJSERIUQCIP4kAZj+aAVV+qtEBM0A//8AIgBYBYEFpxCnAAwFogRQ0sAtPtLA"
                             "0sAQpwAMAX4F2NLA0sAtPtLAEKcADAACAawtPtLALT4tPhCnAAwEJAAoLT4tPtLALT4QpwAM"
                             "/+oEDQAAwABAAAAAEKcADAW+Ae4AAEAAwAAAABAvAAwD3gXswAAQBwAMAcIADAABAFf/4gW7"
                             "BbsAIwAAExA3NiEyBRYVFAcGJwIhIAMGFRQXFiEgEzYXFgcGBwQhIAEmV7jWAY6lATUPEhIG"
                             "oP7k/t+6jZamAWkBM50JGBcCGBv+9/7M/rX+6pECxAEo1vmQB90JAwILATv++8XE9tvyAToS"
                             "BQQSzRGfARGOAAABABT/+gJ8BbQAIwAAMyInJjc2NxI3NgMmJyY3NjMkJRYXFgcGBwIXFhMW"
                             "FxYXFgcGNxcBARefBA0BARUJoBUBARUBIgEIGwEBG7YEDAICDAO4HQIBH/8MCgg+aQFPvqYB"
                             "XJUXAxUSBAYBFw0HNYX+u7y1/qh1HwUZDQEGAAACADz/7wN5A5EACAAuAAA3Fjc2JyYHDgI+"
                             "AycmJyYHBhcWBwYnJjc2MzIDAhcWNzY3NgcGBwYnBicmJ+IDjJYDATJLpqRFkImHAgJAKE5z"
                             "BAVyIhAJHbLN6hcUBAVNQA4qDCqZZVKQbLYEw4UND9pgDxNUO2YoLC6NfjgiBAZBOCMKLhwc"
                             "q/7J/vRgjxcTAwoifQUDdXUBAq4AAQA0//8ETgO2ADMAADMiNTQzMgMmNzYnNjMyBwYHJDc2"
                             "ExIXFjcyFRQjMCEiNTQ3NicwAyYHBgcwAwI3NhcWJyBQHDBkDQYBAUueQDoSFQIBBovUBwkD"
                             "AmcSFf6mJSFHAgUB2XpbCQ5qLQMDDv7SHhUBlbxgTCFlLzc1dAQH/ur+oo9oARoWIxgHEUQB"
                             "2MoJBUP+cv7cBQIcIgEAAQRP/+4GiQObACUAACUmNzYzMhcWNzY3NicmNzY3NhcWBwYnJicm"
                             "BwYHBhcWFxYHBiUmBFUGCAMVFAxWbJcLBqzgGiv3bWQPBgEXFA5lPGEpHJlKTFQFCf7c1zM6"
                             "VBwcug4To01ph5P6BAI4EogUBAQYoAIDkmRmMkNJg+UBAQABADz/7AKEBBEAIwAAEyYnJjc2"
                             "NzYXFgcGFxY3FhUUBwYnJgcCFxYXFjcGJyYTEjU0aCIGBBxcQhUKIAMIVD+VMjKMTk8BCAgJ"
                             "oVVJOc3zERQDLgUXEBZKQhUECyBQAgEHCi41AwcBAVH+u4mnAQEnlAQFAQEBNKpSAAH/+f+6"
                             "A7QDjAAeAAAlJgEmJwUyFRQHBhUUEzYTNicmJzQ3NjcGBwAHBgciAbYX/tMRaAFkHh494U93"
                             "Bz4sASikhV8Y/uEJDR4kDoACfSRdAhYSCxZAJv4/LwHaGRIMGhABAgU9Rv1/VHkBAAH/+f5t"
                             "A7QDjAAmAAAlNAEmJwUyFRQHBhUUEzYTNicmJzQ3NjcGBwIHAgcGIyY1Njc2NzYBqv7IEWgB"
                             "ZB4ePeFFgQc+LAEopIVfGOJGngEYOFgBWSAGWixJApYkXQIWEgsWQCb+PygB4RoRDBoQAQIF"
                             "PUb927D+cQM1AVAZGgkNy///ABT/+gLXB/MQZwAMABEC1T/4QAASBgAEAAAAAQEFAyMCxgUe"
                             "AA0AAAE2EzY3NhcWBwYHBicmARAwqBoOWkoSHsKSFBwfA0prASUtAxQYBSf+ohcHBwAAEAAA"
                             "/nAHkAYAAAMABwALAA8AEwAXABsAHwAjACcAKwAvADMANwA7AD8AABAQIBAAECARABAhEAAQ"
                             "IRESESAQABEgEQARIRAAESERExAgEAEQIBEBECEQARAhERMRIBABESARAREhEAERIREBkP5w"
                             "AZD+cAGQ/nABkHABkP5wAZD+cAGQ/nABkHABkP5wAZD+cAGQ/nABkHABkP5wAZD+cAGQ/nAB"
                             "kP5wAZD+cAIAAZD+cAIAAZD+cAIAAZD+cPoAAZD+cAIAAZD+cAIAAZD+cAIAAZD+cPoAAZD+"
                             "cAIAAZD+cAIAAZD+cAIAAZD+cPoAAZD+cAIAAZD+cAIAAZD+cAIAAZD+cAD//wBkADIDIAWq"
                             "ECcAEgAA/qIABAASAAP//wBkAZADIARMEAYAEgAA//8AZAGQAyAETBAGABIAAP//AGQBkAMg"
                             "BEwQBgASAAAAAQBkAZADIARMAAMAABIgECBkArz9RARM/UQAAAAAAAAMAJYAAQAAAAAAAQAF"
                             "AAAAAQAAAAAAAgAHAAUAAQAAAAAAAwAFAAAAAQAAAAAABAAFAAAAAQAAAAAABQALAAwAAQAA"
                             "AAAABgAFAAAAAwABBAkAAQAKABcAAwABBAkAAgAOACEAAwABBAkAAwAKABcAAwABBAkABAAK"
                             "ABcAAwABBAkABQAWAC8AAwABBAkABgAKABdGb250Q1JlZ3VsYXJWZXJzaW9uIDEuMABGAG8A"
                             "bgB0AEMAUgBlAGcAdQBsAGEAcgBWAGUAcgBzAGkAbwBuACAAMQAuADAAAAMAAAAAAAD/ZgBm"
                             "AAAAAAAAAAAAAAAAAAAAAAAAAAA=";

// Simple Base64 decoder.  This is used at startup to decode the string
// literals containing embedded resource data, namely font files in TTF form.
//
void base64_decode(char const* input, std::vector<unsigned char>& output)
{
  int index = 0;
  int data = 0;
  int held = 0;
  while(int symbol = input[index++])
  {
    if(symbol == '=')
      break;
    int value = ('A' <= symbol && symbol <= 'Z' ? symbol - 'A' :
                 'a' <= symbol && symbol <= 'z' ? symbol - 'a' + 26 :
                 '0' <= symbol && symbol <= '9' ? symbol - '0' + 52 :
                 symbol == '+'                  ? 62 :
                 symbol == '/'                  ? 63 :
                                                  0);
    data = data << 6 | value;
    held += 6;
    if(held >= 8)
    {
      held -= 8;
      output.push_back(static_cast<unsigned char>((data >> held) & 0xff));
      data &= (1 << held) - 1;
    }
  }
}
} // namespace fonts
