<?xml version="1.0" encoding="utf-8"?>
<!DOCTYPE TS>
<TS version="2.0" language="cs_CZ">
<context>
    <name>BE::Database</name>
    <message>
        <location filename="database.cpp" line="194"/>
        <source>Added</source>
        <translation>Přidáno</translation>
    </message>
    <message>
        <location filename="database.cpp" line="206"/>
        <source>Updating
Database</source>
        <translation>Obnovuje se
databáze</translation>
    </message>
</context>
<context>
    <name>BE::InfoWidget</name>
    <message>
        <location filename="infowidget.cpp" line="304"/>
        <source>BE::MPC can fetch information about the current artist &amp; album from wikipedia
The heuristic lookup for the best match is done via a google query</source>
        <translation>BE::MPC může natáhnout informace o nynějším umělci a albu z Wikipedie.
Vyhledání nejlepší shody je provedeno pomocí hledání služby Google</translation>
    </message>
    <message>
        <location filename="infowidget.cpp" line="315"/>
        <source>Allow to query Google &amp;&amp; Wikipedia</source>
        <translation>Povolit hledání u Google a na Wikipedii</translation>
    </message>
    <message>
        <location filename="infowidget.cpp" line="411"/>
        <source>&lt;html&gt;&lt;h1&gt;No Cover found :-(&lt;/h1&gt;&lt;p&gt;Covers are only supported for local connections.&lt;/p&gt;&lt;p&gt;The &quot;best&quot; match in the music file path is taken.&lt;/p&gt;&lt;p&gt;In doubt, name it &lt;b&gt;&quot;cover.jpg&quot;&lt;/b&gt; and place it into the &lt;b&gt;album folder&lt;/b&gt;.&lt;/p&gt;&lt;/html&gt;</source>
        <translation>&lt;html&gt;&lt;h1&gt;Nenalezen žádný obal:-(&lt;/h1&gt;&lt;p&gt;Obaly jsou podporovány pouze pro místní spojení.&lt;/p&gt;&lt;p&gt;Bere se nejlepší shoda v cestě k hudebnímu souboru.&lt;/p&gt;&lt;p&gt;V případě pochybností jej pojmenujte &lt;b&gt;&quot;cover.jpg&quot;&lt;/b&gt; a umístěte do &lt;b&gt;složky s albem&lt;/b&gt;.&lt;/p&gt;&lt;/html&gt;</translation>
    </message>
    <message>
        <location filename="infowidget.cpp" line="581"/>
        <source>drop an image</source>
        <translation>Upustit obrázek</translation>
    </message>
</context>
<context>
    <name>BE::MPC</name>
    <message>
        <location filename="mpc.cpp" line="150"/>
        <source>Sorry, no local server</source>
        <translation>Promiňte, žádný místní server</translation>
    </message>
    <message>
        <location filename="mpc.cpp" line="151"/>
        <source>&lt;html&gt;Sorry, you cannot add local files because:&lt;br&gt;&lt;b&gt;You are not connected to a local MPD server&lt;/b&gt;&lt;/html&gt;</source>
        <translation>&lt;html&gt;Promiňte, ale nemůžete přidat místní soubory, protože:&lt;br&gt;&lt;b&gt;nejste připojen k místnímu serveru MPD&lt;/b&gt;&lt;/html&gt;</translation>
    </message>
    <message>
        <location filename="mpc.cpp" line="161"/>
        <source>Do you want to fake local file support?</source>
        <translation>Chcete předstírat podporu místního souboru?</translation>
    </message>
    <message>
        <location filename="mpc.cpp" line="162"/>
        <source>&lt;html&gt;&lt;h1&gt;Do you want to fake local file support?&lt;/h1&gt;The MPD backend does not support to play files from &quot;somewhere on your disk&quot;, however this can be compensated.&lt;br&gt;If you agree, you will get a new directory&lt;br&gt;&lt;center&gt;&lt;b&gt;BE_MPC_LOCAL_FILES&lt;/b&gt;&lt;/center&gt;&lt;br&gt;in your MPD music path where BE::MPC will create links to the actual files so that MPD can find and play them.&lt;br&gt;BE::MPC will remove all unused links as soon as you remove anything from the playlist.&lt;br&gt;&lt;center&gt;It&apos;s usually save to say &quot;Yes&quot;&lt;/center&gt;&lt;/html&gt;</source>
        <translation>&lt;html&gt;&lt;h1&gt;Chcete předstírat podporu místního souboru?&lt;/h1&gt;Jádro MPD nepodporuje přehrávání souborů &quot;odkudkoli na vašem disku&quot;, nicméně to lze vynahradit.&lt;br&gt;Pokud souhlasíte, dostanete nový adresář&lt;br&gt;&lt;center&gt;&lt;b&gt;BE_MPC_LOCAL_FILES&lt;/b&gt;&lt;/center&gt;&lt;br&gt;ve vaší cestě k hudbě MPD, kde BE::MPC vytvoří odkazy ke skutečným souborům, takže je MPD může najít a přehrát.&lt;br&gt;BE::MPC odstraní všechny nepoužívané odkazy, jakmile cokoli odstraníte ze svého seznamu skladeb.&lt;br&gt;&lt;center&gt;Obvykle je bezpečné říct &quot;Ano&quot;&lt;/center&gt;&lt;/html&gt;</translation>
    </message>
    <message>
        <location filename="mpc.cpp" line="179"/>
        <source>Sorry, no write permission</source>
        <translation>Promiňte, žádná oprávnění pro zápis</translation>
    </message>
    <message>
        <location filename="mpc.cpp" line="180"/>
        <source>&lt;html&gt;Sorry, you cannot add local files because:&lt;br&gt;You have no permission to write to&lt;br&gt;&lt;center&gt;&lt;b&gt;%1&lt;/b&gt;&lt;/center&gt;&lt;/html&gt;</source>
        <translation>&lt;html&gt;Promiňte, místní soubory nemůžete přidat, protože:&lt;br&gt;Nemáte žádná oprávnění pro zápis do&lt;br&gt;&lt;center&gt;&lt;b&gt;%1&lt;/b&gt;&lt;/center&gt;&lt;/html&gt;</translation>
    </message>
    <message>
        <location filename="mpc.cpp" line="851"/>
        <location filename="mpc.cpp" line="853"/>
        <location filename="mpc.cpp" line="1685"/>
        <location filename="mpc.cpp" line="1687"/>
        <source>Search database...</source>
        <translation>Hledat v databázi...</translation>
    </message>
    <message>
        <location filename="mpc.cpp" line="865"/>
        <source>Shuffle</source>
        <translation>Zamíchat</translation>
    </message>
    <message>
        <location filename="mpc.cpp" line="872"/>
        <location filename="mpc.cpp" line="911"/>
        <source>Genre</source>
        <translation>Žánr</translation>
    </message>
    <message>
        <location filename="mpc.cpp" line="872"/>
        <location filename="mpc.cpp" line="911"/>
        <source>Artist</source>
        <translation>Umělec</translation>
    </message>
    <message>
        <location filename="mpc.cpp" line="873"/>
        <location filename="mpc.cpp" line="912"/>
        <source>AlbumArtist</source>
        <translation>Umělec alba</translation>
    </message>
    <message>
        <location filename="mpc.cpp" line="873"/>
        <location filename="mpc.cpp" line="912"/>
        <source>Album</source>
        <translation>Album</translation>
    </message>
    <message>
        <location filename="mpc.cpp" line="874"/>
        <location filename="mpc.cpp" line="913"/>
        <source>Disc</source>
        <translation>Disk</translation>
    </message>
    <message>
        <location filename="mpc.cpp" line="874"/>
        <source>Track</source>
        <translation>Skladba</translation>
    </message>
    <message>
        <location filename="mpc.cpp" line="874"/>
        <location filename="mpc.cpp" line="913"/>
        <source>Year</source>
        <translation>Rok</translation>
    </message>
    <message>
        <location filename="mpc.cpp" line="875"/>
        <source>Title</source>
        <translation>Název</translation>
    </message>
    <message>
        <location filename="mpc.cpp" line="882"/>
        <source>Repeat current track</source>
        <translation>Opakovat současnou skladbu</translation>
    </message>
    <message>
        <location filename="mpc.cpp" line="888"/>
        <source>Random track</source>
        <translation>Náhodná skladba</translation>
    </message>
    <message>
        <location filename="mpc.cpp" line="900"/>
        <source>Repeat playlist</source>
        <translation>Opakovat seznam skladeb</translation>
    </message>
    <message>
        <location filename="mpc.cpp" line="1403"/>
        <source>Unknown</source>
        <translation>Neznámý</translation>
    </message>
    <message>
        <location filename="mpc.cpp" line="1474"/>
        <source>Broadcast: Internet Radio</source>
        <translation>Vysílání: Internetové rádio</translation>
    </message>
    <message>
        <location filename="mpc.cpp" line="1653"/>
        <location filename="mpc.cpp" line="1655"/>
        <source>Double click to return to playlist</source>
        <translation>Klepnout dvakrát pro návrat do seznamu skladeb</translation>
    </message>
</context>
<context>
    <name>BE::MpdSettings</name>
    <message>
        <location filename="mpd_settings.cpp" line="65"/>
        <source>Sound</source>
        <translation>Zvuk</translation>
    </message>
    <message>
        <location filename="mpd_settings.cpp" line="66"/>
        <source>User Interface</source>
        <translation>Uživatelské rozhraní</translation>
    </message>
    <message>
        <location filename="mpd_settings.cpp" line="70"/>
        <source>Configure MPD connection</source>
        <translation>Nastavit spojení MPD</translation>
    </message>
    <message>
        <location filename="mpd_settings.cpp" line="84"/>
        <source>New</source>
        <translation>Nový</translation>
    </message>
    <message>
        <location filename="mpd_settings.cpp" line="85"/>
        <source>Delete</source>
        <translation>Smazat</translation>
    </message>
    <message>
        <location filename="mpd_settings.cpp" line="87"/>
        <location filename="mpd_settings.cpp" line="198"/>
        <location filename="mpd_settings.cpp" line="219"/>
        <location filename="mpd_settings.cpp" line="264"/>
        <location filename="mpd_settings.cpp" line="266"/>
        <location filename="mpd_settings.cpp" line="359"/>
        <location filename="mpd_settings.cpp" line="403"/>
        <source>Local</source>
        <translation>Místní</translation>
    </message>
    <message>
        <location filename="mpd_settings.cpp" line="95"/>
        <source>Server</source>
        <translation>Server</translation>
    </message>
    <message>
        <location filename="mpd_settings.cpp" line="99"/>
        <source>Password</source>
        <translation>Heslo</translation>
    </message>
    <message>
        <location filename="mpd_settings.cpp" line="103"/>
        <source>Reconnect</source>
        <translation>Znovu připojit</translation>
    </message>
    <message>
        <location filename="mpd_settings.cpp" line="109"/>
        <source>Local daemon configuration</source>
        <translation>Nastavení místního démona</translation>
    </message>
    <message>
        <location filename="mpd_settings.cpp" line="116"/>
        <source>Music directory</source>
        <translation>Adresář s hudbou</translation>
    </message>
    <message>
        <location filename="mpd_settings.cpp" line="123"/>
        <source>Rescan...</source>
        <translation>Prohlédnout znovu...</translation>
    </message>
    <message>
        <location filename="mpd_settings.cpp" line="125"/>
        <source>Database</source>
        <translation>Databáze</translation>
    </message>
    <message>
        <location filename="mpd_settings.cpp" line="127"/>
        <source>Playlists directory</source>
        <translation>Adresář se seznamy skladeb</translation>
    </message>
    <message>
        <location filename="mpd_settings.cpp" line="129"/>
        <source>Log</source>
        <translation>Zápis</translation>
    </message>
    <message>
        <location filename="mpd_settings.cpp" line="131"/>
        <source>MPD state</source>
        <translation>Stav MPD</translation>
    </message>
    <message>
        <location filename="mpd_settings.cpp" line="134"/>
        <source>Autostart, if not present</source>
        <translation>Spustit automaticky, není-li přítomný</translation>
    </message>
    <message>
        <location filename="mpd_settings.cpp" line="140"/>
        <source>The local daemon could not be connected</source>
        <translation>Místního démona se nepodařilo připojit</translation>
    </message>
    <message>
        <location filename="mpd_settings.cpp" line="143"/>
        <source>Start MPD</source>
        <translation>Spustit MPD</translation>
    </message>
    <message>
        <location filename="mpd_settings.cpp" line="149"/>
        <source>Touchscreen mode</source>
        <translation>Režim dotykové obrazovky</translation>
    </message>
    <message>
        <location filename="mpd_settings.cpp" line="150"/>
        <source>Autoswitch between Info and Playlist</source>
        <translation>Automaticky přepínat mezi informacemi a seznamem skladeb</translation>
    </message>
    <message>
        <location filename="mpd_settings.cpp" line="151"/>
        <source>Hint keyboard shortcuts</source>
        <translation>Radit klávesové zkratky</translation>
    </message>
    <message>
        <location filename="mpd_settings.cpp" line="152"/>
        <source>Cover in Colors</source>
        <translation>Obal v barvách</translation>
    </message>
    <message>
        <location filename="mpd_settings.cpp" line="153"/>
        <source>Playlist indicator contrast</source>
        <translation>Kontrast ukazatele seznamu skladeb</translation>
    </message>
    <message>
        <location filename="mpd_settings.cpp" line="157"/>
        <source>Allow Google/Wikipedia queries for tune details</source>
        <translation>Povolit hledání na Google/Wikipedia pro podrobnosti o písničce</translation>
    </message>
    <message>
        <location filename="mpd_settings.cpp" line="158"/>
        <source>Panning infotext requires ctrl+alt to be pressed</source>
        <translation>Přejíždění informačního textu vyžaduje stisknutí klávesové zkratky Ctrl+Alt</translation>
    </message>
    <message>
        <location filename="mpd_settings.cpp" line="162"/>
        <source>Ok</source>
        <translation>OK</translation>
    </message>
    <message>
        <location filename="mpd_settings.cpp" line="225"/>
        <source>Select music base directory</source>
        <translation>Vybrat základní adresář s hudbou</translation>
    </message>
    <message>
        <location filename="mpd_settings.cpp" line="231"/>
        <source>Select MPD database file</source>
        <translation>Vybrat soubor s databází MPD</translation>
    </message>
    <message>
        <location filename="mpd_settings.cpp" line="237"/>
        <source>Select playlist storage directory</source>
        <translation>Vybrat ukládací adresář pro seznam skladeb</translation>
    </message>
    <message>
        <location filename="mpd_settings.cpp" line="243"/>
        <source>Select MPD log file</source>
        <translation>Vybrat soubor se zápisem MPD</translation>
    </message>
    <message>
        <location filename="mpd_settings.cpp" line="249"/>
        <source>Select MPD state file (allows restoring of playlist &amp; position)</source>
        <translation>Vybrat soubor o stavu MPD (umožňuje obnovu seznamu skladeb a polohy)</translation>
    </message>
    <message>
        <location filename="mpd_settings.cpp" line="498"/>
        <source>Select music base (sub)directory to rescan</source>
        <translation>Vybrat základní (pod)adresář s hudbou k prohledání</translation>
    </message>
    <message>
        <location filename="mpd_settings.cpp" line="500"/>
        <source>Invalid Path</source>
        <translation>Neplatná cesta</translation>
    </message>
    <message>
        <location filename="mpd_settings.cpp" line="500"/>
        <source>You must select a (sub)directory of the current Music directory)</source>
        <translation>Musíte vybrat (pod)adresář nynějšího hudebního adresáře</translation>
    </message>
</context>
<context>
    <name>BE::Player</name>
    <message>
        <location filename="player.cpp" line="60"/>
        <source>Volume</source>
        <translation>Hlasitost</translation>
    </message>
</context>
<context>
    <name>BE::Playlist</name>
    <message>
        <location filename="playlist.cpp" line="186"/>
        <source>Type to filter</source>
        <translation>Typ k filtrování</translation>
    </message>
</context>
<context>
    <name>BE::PlaylistManager</name>
    <message>
        <location filename="playlistmanager.cpp" line="39"/>
        <location filename="playlistmanager.cpp" line="54"/>
        <source>Enter new playlist name...</source>
        <translation>Zadat nový název seznamu skladeb...</translation>
    </message>
    <message>
        <location filename="playlistmanager.cpp" line="45"/>
        <source>Delete</source>
        <translation>Smazat</translation>
    </message>
</context>
<context>
    <name>BE::Sorter</name>
    <message>
        <location filename="sorter.cpp" line="109"/>
        <source>Remove</source>
        <translation>Odstranit</translation>
    </message>
    <message>
        <location filename="sorter.cpp" line="114"/>
        <source>Clear</source>
        <translation>Vyprázdnit</translation>
    </message>
</context>
</TS>
