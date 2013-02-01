#include "Transfer.hpp"
#include "Viewer.hpp"

using namespace webotsQtUtils;

Viewer::Viewer() :
  CategoryViewer()
{
  mTabWidget->addTab(new Transfer, QString("Transfer"));
}

Viewer::~Viewer() {
}