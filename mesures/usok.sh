#!/usr/bin/env bash
# USok — Unité Sokoban : unité de TEMPS normalisée par machine, à la manière des
# SBU (Standard Build Unit) de Linux From Scratch.
#
# Pourquoi. Le projet tourne sur plusieurs machines. Un temps en secondes écrit
# dans plan.md/scores.md ne veut donc rien dire (il dépend de la machine qui l'a
# mesuré) — exactement le piège « ne jamais comparer à un chiffre écrit » du §1.
# Un temps en USok, lui, est portable : c'est un RAPPORT à un étalon re-mesuré sur
# place à chaque appel.
#
#   1 USok = temps de résolution de l'ÉTALON `bench 2 astar`
#            (A* pur, 590066 états / 131 poussées).
#
# L'étalon est en A* PUR : il n'emprunte jamais le chemin macro/corral/
# goal-ordering, donc son nombre d'états est un invariant du canari et il ne
# DÉRIVE pas quand on travaille sur ces parties. C'est un mètre fixe.
#
# Usage :
#   usok.sh <niv> <mode>            # rend la cible en USok (calibre l'étalon d'abord)
#   CORRAL=1 usok.sh 17 macro       # coût du corral incrémental sur la cible
#   USOK_REF=14.9 usok.sh 7 macro   # réutilise une calibration connue (saute l'étalon)
#   USOK_N=5 usok.sh 3 macro        # meilleur de N au lieu de 3
#
# ⚠️ Les variables CORRAL/LIVRAISON de l'appelant sont transmises à la CIBLE mais
# JAMAIS à l'étalon (forcé à 0) : sinon l'étalon changerait avec l'expérience et ne
# serait plus un mètre fixe.
#
# Portable Linux/macOS : chronométrage via le builtin bash `time` (%R), pas
# /usr/bin/time (dont l'option -f est propre à GNU et absente sur macOS, cf. §1).
set -u

# Point décimal, pas la virgule : sur une machine en locale fr_FR, `time` rend
# "15,341" et awk le lit comme deux champs → division fausse. C affecte seulement
# le formatage des nombres, jamais le solveur.
export LC_ALL=C

ETALON_NIV=2
ETALON_MODE=astar
N=${USOK_N:-3}          # meilleur de N (le §1 impose au moins 3)

BIN="$(cd "$(dirname "$0")" && pwd)/build/bench/bench"
if [ ! -x "$BIN" ]; then
    echo "bench introuvable : $BIN" >&2
    echo "  construire d'abord : (cd mesures/build/bench && qmake ../../bench.pro && make)" >&2
    exit 1
fi

# Meilleur (min) de N chronos du temps réel, en secondes. La jauge de progression
# du solveur part sur stderr (§1) : on la jette, sinon elle se mêle à la sortie de
# `time`. La dernière ligne stdout (le "OK ... etats=...") est renvoyée pour info.
# Renvoie "<secondes>|<ligne bench>".
chrono() {   # args : commande complète (préfixe env compris)
    local best="" t of
    of=$(mktemp 2>/dev/null || mktemp -t usok)
    local i
    for i in $(seq "$N"); do
        t=$( { TIMEFORMAT='%R'; time "$@" >"$of" 2>/dev/null ; } 2>&1 )
        if [ -z "$best" ] || awk "BEGIN{exit !($t<$best)}"; then best=$t; fi
    done
    printf '%s|%s' "$best" "$(tail -1 "$of")"
    rm -f "$of"
}

niv=${1:?usage: usok.sh <niv> <mode>}
mode=${2:-astar}

if [ -n "${USOK_REF:-}" ]; then
    ref=$USOK_REF
    printf '1 USok = %ss   (réutilisé via USOK_REF, étalon non re-mesuré)\n' "$ref"
else
    r=$(chrono env CORRAL=0 LIVRAISON=0 CORRAL_DETECT=0 "$BIN" "$ETALON_NIV" "$ETALON_MODE")
    ref=${r%%|*}
    printf '1 USok = %ss   [étalon : bench %s %s — %s, meilleur de %s]\n' \
           "$ref" "$ETALON_NIV" "$ETALON_MODE" "${r#*|}" "$N"
fi

c=$(chrono "$BIN" "$niv" "$mode")
tsec=${c%%|*}
tline=${c#*|}
usok=$(awk "BEGIN{printf \"%.3f\", $tsec/$ref}")
# Corral ACTIF par défaut (promu) ; seul CORRAL=0 le coupe (cf. solveurastar.cpp).
corral=$([ "${CORRAL:-}" = "0" ] && echo off || echo on)
printf 'cible  : bench %s %s (corral %s) — %ss = %s USok   [%s]\n' \
       "$niv" "$mode" "$corral" "$tsec" "$usok" "$tline"
