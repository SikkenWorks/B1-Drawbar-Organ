# FM1 Cross-Firmware OTA Compatibility — technisch audit- en herstelprotocol

**Toepassen op:** BEETLE, B1, NERVE en toekomstige M-VAVE FM1-firmware.  
**Referentie:** ETCH V1.38 → V1.49 OTA-blokkade; ETCH V1.50 gedeeltelijke native oplossing.  
**Doel:** zorgen dat een geïnstalleerde custom firmware de gebruiker niet onbedoeld vasthoudt en veilig naar aantoonbaar compatibele custom firmware of stock kan overstappen.

> **Status:** auditopdracht, geen claim dat BEETLE, B1 of NERVE momenteel deze bug hebben. Daarvoor moeten hun actuele native broncode, OTA-builds en originele .fwsc-pakketten apart onderzocht worden. Ook ETCH V1.50 is nog niet met alle originele Felucca-pakketten en echte hardware bewezen.

## 1. Het oorspronkelijke probleem

Na installatie van **ETCH V1.38 (`FM-1_1038`)** kon de FM1 niet rechtstreeks naar bepaalde andere custom firmwares updaten. Een externe WebInstaller, Felucca-installer of Node-updater kon verbinding maken en begon de eerste firmwarecontrole (`CHECK`), maar **de op de FM1 draaiende ETCH-updater brak de overdracht zelf af**, nog vóór de host zijn speciale leesverzoek `0xE0000000` met antwoord `success` kon afhandelen. Er werd geen nieuwe firmware geschreven; de FM1 bleef ETCH draaien.

Gedocumenteerd gebruikersgedrag:

- **Felucca 1.6:** verzoeken voor `0x0`, `0x200` (UFW-header), `0x93400` (loaderheader) en loaderbody `0x93420…0x94e20`, daarna stop. De door gebruiker gerapporteerde loaderbody is **6.898 bytes**. Geen succes-handshake of OTA-herstart.
- **Felucca 1.1.5:** vergelijkbare 17 pakketten, daarna verzoek voor `0x400` (begin `flash.bin`), vervolgens stop. Geen succes-handshake.
- De gebruiker toonde een **werkbare, aangepaste Felucca 1.6-applicatie in een ETCH 1.38-compatibele container**. Dit ondersteunt de hypothese dat de app zelf op dezelfde flashindeling kan draaien; het bewijst niet dat iedere originele firmware of loader veilig is.
- De eerdere ETCH `V1.38 + OTA Bug FIX` bleek geen directe Felucca-update mogelijk te maken. De stock-V15-herstelbrug vormde een **indirecte** route: ETCH → officieel V15 → Felucca. Dat is geen oplossing voor rechtstreekse cross-firmware-updates en was niet fysiek gegarandeerd.

## 2. Technische hoofdoorzaken

### A. Verkeerde bootheadercontrole voor een app-only update

Een app-only-loader kan de beschermde bootsector `0x0000–0x3FFF` ongemoeid laten. In dat geval is het onjuist een **andere maar ongebruikte bootheader** als enige afwijsreden te gebruiken. Oudere ETCH-versies deden dat wel (`-5`). Een geldig pakket kan daardoor worden afgewezen voordat de daadwerkelijke applicatie op compatibiliteit is gecontroleerd.

**Correctie:** maak expliciet onderscheid tussen `APP_ONLY` en `FULL_FLASH`. Bij `APP_ONLY` moet de **bestaande, beschermde bootheader ongewijzigd blijven**; de pakketheader is geen vereiste byte-match wanneer die niet wordt geschreven. Bij `FULL_FLASH` mag de bootheader alleen worden gewijzigd via een afzonderlijk, volledig gevalideerd herstelpad.

### B. Een te strenge of verkeerde loader-allowlist

Oudere ETCH-updaters accepteerden alleen een specifieke loaderbody op basis van lengte en SHA-256 (`-16`). Daardoor kon een op zichzelf compatibel pakket met een andere loader worden afgewezen. Maar **blind alle loaders toestaan is onveilig**, want de loader wordt uitgevoerd met flashschrijfrechten.

**Correctie:** gebruik waar mogelijk een **eigen vertrouwde, gecontroleerde app-only-loader**, onafhankelijk van de meegeleverde loaderbody. Controleer de container van de inkomende loader op consistentie, maar voer de onbekende body niet uit. Verifieer dat de vertrouwde loader werkelijk de beoogde doelapplicatie, offsets en USB-MIDI-overdracht ondersteunt.

### C. Verkeerde identiteit tijdens de OTA-overgang

Normale firmware presenteert bijvoorbeeld `FM-1_900` of `FM-1_1038`. Na CHECK kan een OTA-loader opnieuw op USB-MIDI verschijnen met een **andere identiteit**, bijvoorbeeld `ota-FM-1_015` of `ota-FM-1_900`. Een installer die alleen de normale poortnaam herkent, of die een identiteit verwacht die niet bij de werkelijk gestagede loader hoort, blijft hangen bij `WAITING FOR OTA LOADER`.

**Correctie:** de **gestagede, daadwerkelijk uitgevoerde loader** bepaalt de verwachte identiteit. De installer moet opnieuw detecteren, expliciet de identiteit opvragen en alleen naar WRITE gaan na een bevestigde ondersteunde loaderhandshake. Een gevonden poortnaam als `Felucca Update` of de normale identiteit `FM-1_900` is op zichzelf onvoldoende.

### D. Een geslaagde WebInstaller-test is geen geslaagde native CHECK-test

Browsertests kunnen een `CHECK OK` simuleren zonder de C-code te draaien die het originele `.fwsc`-pakket daadwerkelijk valideert. Dat verklaart waarom meerdere ETCH-installers in simulaties goed leken, terwijl echte updates vastliepen.

**Correctie:** test de **exacte native firmware-updater** met de **exacte externe `.fwsc`-bestanden**, inclusief alle `READ`-verzoeken, de succeshandschake, staging, resetmetadata, loaderhandshake en flash-write-simulatie.

## 3. Wat ETCH V1.50 verandert en wat nog bewezen moet worden

In ETCH V1.50 staat de aanpassing in `firmware/fm1-native/ota.c`:

1. Controle van UFW-header/entrylist en pakketgrenzen (`ota_ufw`, `ota_stage`).
2. App-only-pakketten worden niet meer geweigerd puur vanwege een afwijkende beschermde bootheader.
3. De inkomende app-layout wordt getoetst: onder andere chip key `0x980F`, app-flashoffset `0x4000`, XIP-adres `0x02000120`, app-directory, configuratie-CRC en grootte (`etch_host_app_compatible`).
4. De originele inkomende OTA-loader wordt **niet uitgevoerd**. In plaats daarvan wordt de SHA-gepinde ingebouwde ETCH-loader gestaged, en de flashinhoud daarvan teruggelezen/gecontroleerd.
5. De app-only-loader meldt `ota-FM-1_015` omdat dát de identiteit van de werkelijk gestagede loader is. Niet automatisch `ota-FM-1_900`.
6. Volledige stock V15-restore blijft een afzonderlijk, exact gecontroleerd pad.
7. De host moet de speciale `0xE0000000`-leesaanvraag met de 8 bytes ASCII `success` beantwoorden voordat de OTA-commit doorgaat.

**Grenzen van V1.50:** de code vereist bij app-only updates nog een specifieke UFW-indeling (`flash.bin` onder meer `0x93000` bytes), een **loaderbody van 6.898 bytes**, specifieke loaderheaderwaarden en een specifieke app-layout. Daardoor is 'alle soorten custom firmware' geen technisch bewezen claim. Een oorspronkelijke Felucca-update met afwijkende loaderlengte, header of layout kan nog steeds worden geweigerd. De originele Felucca 1.6/1.1.5-pakketten en de overstap op echte hardware zijn nog niet end-to-end bevestigd.

## 4. Verplichte audit van BEETLE, B1 en NERVE

Controleer **afzonderlijk per project**, in de **daadwerkelijk gecompileerde native firmware** (niet uitsluitend WebInstaller of simulator):

1. **OTA-ingang:** welk SysEx-commando begint CHECK, hoe worden pakketreeksen en readrequests ontvangen, hoe werkt de timeout en annulering?
2. **UFW-verificatie:** CRC, entrylist, offsets, bestandsgrootte, toegestane gebieden, integer overflow, overlappende entries, ontbrekende/verkeerde header, duplicate entries en misvormde antwoorden.
3. **Beslissing APP_ONLY versus FULL_FLASH:** welke flashbereiken worden werkelijk beschreven? Wordt het beschermde bootgedeelte vóór iedere erasure geïsoleerd? Is er een onterechte bytevergelijking van het vreemde flash-head?
4. **Loaderbeleid:** welke loader wordt gestart, waar komt die vandaan, hoe wordt authenticiteit/integriteit vastgesteld, en is zijn gedrag/flashbereik daadwerkelijk bekend? Nooit een willekeurig geüploade executable starten.
5. **Doelapp-compatibiliteit:** chip key, flashcapaciteit, app-base/offset, XIP-layout, SFC/CRC, app-grootte, ROM-/SDK-vereisten, opslaggebieden en mogelijke conflicten met projectdata.
6. **USB-MIDI-overgang:** identity van normale firmware, werkelijk gestagede OTA-loader en nieuwe firmware; gebruik niet alleen MIDI-poortnamen of een hardcoded verwachting die bij de gestagede loader niet klopt.
7. **CHECK/success/commit:** `0xE0000000`-handshake, exacte bevestigingsbytes, staging/write/readback, commit-record, reboot. Flash nooit wanneer CHECK of hostbevestiging ontbreekt.
8. **Stockherstel:** officiële V15 als afzonderlijk gecontroleerde herstelroute. Maak geen algemene 'full flash'-optie met willekeurige bootimages.
9. **Rollback/fail-safe:** timeout, USB-onderbreking, power loss, CRC-fout, onverwachte loader, flash-erase-/writefout en veilige cleanup zonder ongewenst wissen van vreemde projectsectoren.
10. **Release-artifacts:** zijn de `.fwsc`-bestanden, installer-embedded packages, hashes, native binaries, versie-identiteit, broncode en CHANGELOG onderling daadwerkelijk consistent?

**Speciale projectvraag:** BEETLE, B1 en NERVE kunnen andere flashsectoren reserveren voor patronen, projecten of samples. Een technisch compatibele `app.bin` mag nooit betekenen dat projectopslag, OTA-staging of recoverygegevens veilig zijn zonder een project-specifieke geheugenkaart en overlapcontrole.

## 5. Regressietests die iedere firmware moet hebben

Gebruik waar mogelijk *originele, ongewijzigde* `.fwsc`-pakketten: actuele Felucca, officiële M-VAVE V15, ETCH, BEETLE, B1 en NERVE. Test minstens:

| Scenario | Vereiste uitkomst |
|---|---|
| Eigen firmware → nieuwere eigen firmware | Geslaagde volledige native CHECK en veilige OTA-overgang |
| Eigen firmware → oudere compatibele eigen firmware | Geslaagd of expliciet onderbouwd geblokkeerd, nooit stil vastlopen |
| Eigen firmware → compatibele vreemde app-only firmware, andere flash-head | Geen afwijzing uitsluitend wegens niet-beschreven flash-head |
| Eigen firmware → vreemde loaderbody | Geen uitvoering van ongeverifieerde loaderbody; alleen veilige vertrouwde route |
| Eigen firmware → officiële V15 | Alleen na exact geverifieerde full-flash-restorecontrole |
| Felucca 1.6 en 1.1.5 origineel | Exacte native packet-replay: bevestig alle reads en waar de CHECK wel/niet doorgaat |
| Onjuist chip key/offset/layout/grootte/CRC | Weigeren vóór staging of flashwrite, met duidelijke foutcode |
| Onbekend loaderprotocol of andere hardware | Veilig stoppen, geen willekeurige WRITE |
| USB-uitval/timeout/annulering bij elk stadium | Geen onbedoelde commit; bestaande firmware behoudt herstelroute |
| Volledig flashscenario met gemanipuleerde bootheader | Weigeren, behalve expliciete geverifieerde stockroute |
| Test op originele fysieke FM1 | Reboot, eindidentiteit, functiecontrole en mogelijkheid om weer weg te updaten |

**Voor elke test opslaan:** bronversie/identity, bestemming `.fwsc`-SHA-256, loader-SHA-256, readrequest-volgorde/adressen, native foutcode, resultaat handshake `0xE0000000`, gestagede loaderidentiteit, toegestane schrijfsectoren, geëmuleerde flashdiff en eindidentiteit.

## 6. Implementatievoorwaarden en oplevering

- Verhelp de fout **in de native firmware**; alleen de browserinstaller aanpassen volstaat niet.
- Scheid `APP_ONLY` en `FULL_FLASH` expliciet en behoud beveiliging van de bootloader.
- Gebruik een bewezen, gecontroleerde OTA-writer voor app-only updates; verifieer eigen staging en de daadwerkelijke flashgrenzen.
- Ondersteun andere firmwarepakketten **alleen nadat** hun layout en overdrachtsprotocol bewezen compatibel zijn. Implementeer eventueel expliciete adapters per pakketfamilie, in plaats van een onbeperkte wildcard.
- Houd de hostinstaller tolerant voor USB-herenumeratie en verschillende loaderidentiteiten, maar strikt voor daadwerkelijke handshakes.
- Voeg native replaytests toe die de **bestaande defecte ETCH V1.38-gevallen** als regressie bewaren.
- Lever een **volledige schone broncode-ZIP**, nieuw gecompileerde OTA-firmware, werkende installer, het testscript met resultaten en een duidelijke lijst nog onbewezen fysieke scenario's.
- Houd in de hoofdmap uitsluitend `README.md` (uitleg, bediening, alle credits) en `CHANGELOG.md` (versiegeschiedenis, wijzigingen en bugfixes) voor de gebruikersdocumentatie. Bewaar noodzakelijke licenties en ontwikkeltests.

## 7. Acceptatiecriteria voor 'opgelost'

De bug is **niet** opgelost alleen omdat een WebInstaller `CHECK OK` kan simuleren, een `.fwsc` correct is verpakt of stock V15 als tussenstation werkt.

Pas als de betreffende **native broncode** de echte, compatibele externe firmwarepakketten op hostsimulatie verwerkt zonder onterechte afwijzing; een veilige loader correct staged; de echte FM1 de firmwarewissel uitvoert; en vervolgens **weer naar een andere firmware kan updaten**, kan de rechtstreeks-overschakelen-bug als hardwarematig verholpen worden beschouwd.

**Let op bij oude installaties:** een FM1 waarop nog een oudere, blokkerende firmware draait, blijft diens oude CHECK-code gebruiken. Een nieuwe versie kan die niet op afstand veranderen zolang de oude versie het installeren weigert. Documenteer daarvoor een afzonderlijk gevalideerde tussenstap of herstelroute.
