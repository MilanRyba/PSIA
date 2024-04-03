### Jak to postavit a spusit?

Běž do složky Build a spusť premake_vs2022.bat. To vygeneruje Visual Studio solution a project files.
V root directory pak otevři PSIA.sln. Postav celé řešení - Ctrl+Shift+B a nebo right-clickni na řešení (solution) v průzkumníkovi řešení (solution explorer)

**!! Musíš nastavit, aby se spustil jak Sender, tak i Receiver. Right-clikni na řešení -> vlastnosti a zaškrtni 'Více projektů po spuštění a nastav 'Spustit' u obou. Ne u PSIA !!**

F5 spustí program (taky zkompiluje co je případně potřeba)

**!! Důležité také je, aby si klikla na nějaký projekt a v 'hlavičce' průzkumníka řešení klikla na 'zobrazit všechny soubory'. Tohle udělej pro všechny projekty. !!**

### Co je Sender, Receiver a PSIA?

Sender a Receiver jsou aplikace (postaví se jako .exe). PSIA se postaví jako statická knihovna, která je linknutá do Sendera a Receivera.

### Proč jsem udělal statickou knihovnu?

Jelikož obě aplikace využívají do jisté míry stejný kód (definice Packetu a jiné užitečné věci) a
je to hodně špatnej nápad mít dva stejný soubory ve dvou projektech, které edituješ. Knihovnu tak můžeme linknout do obou aplikací a hotovo. 

Jako u většiny tady, netuším co přesně po nás chtějí a jak máme ten kód psát, a tak tohle je dočasný řešení :)

Sender má v sobě napsanou classu Socket. To jsem jen něco zkoušel a není to jestě (pokud vůbec) v Receiveru. Sender má také složku Resources, kde jsou nějaké testovací soubory.