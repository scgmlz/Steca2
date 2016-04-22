# usage: make_pro.sh <extra CONFIG>

APP=$(basename `pwd`)
PRO=$APP.pro
CFG=$*

echo -e '# generated project\n' > $PRO

cat >> $PRO <<EOT
TARGET   = $APP
TEMPLATE = app

QT      += core gui widgets
CONFIG  += silent $CFG
DEFINES += STECA_LABS

INCLUDEPATH += \$\$PWD/core \$\$PWD/gui
LIBS    += -lGLU
EOT

function files {
  where=$1; ext=$2
  find $where -type f -name \*.$ext -exec echo ' ' {} \\ \;
}

echo -e '\nHEADERS += \\'	>> $PRO
files core h			>> $PRO
files gui  h			>> $PRO

echo -e '\nSOURCES += \\'	>> $PRO
files core cpp			>> $PRO
files gui  cpp			>> $PRO

echo -e '\nRESOURCES += \\'	>> $PRO
files core qrc			>> $PRO
files gui  qrc			>> $PRO

