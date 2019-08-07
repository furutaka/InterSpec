#include "InterSpec_config.h"

#include <cmath>
#include <string>
#include <vector>
#include <numeric>
#include <assert.h>
#include <algorithm>

#include <Wt/WText>
#include <Wt/WTable>
#include <Wt/WLabel>
#include <Wt/WCheckBox>
#include <Wt/WResource>
#include <Wt/WLineEdit>
#include <Wt/WModelIndex>
#include <Wt/WGridLayout>
#include <Wt/WPushButton>
#include <Wt/WApplication>
#include <Wt/Http/Request>
#include <Wt/Http/Response>
#include <Wt/WRegExpValidator>
#include <Wt/WContainerWidget>
#include <Wt/WAbstractItemModel>
#include <Wt/WAbstractItemDelegate>

#include "InterSpec/FluxTool.h"
#include "InterSpec/InterSpec.h"
#include "InterSpec/AuxWindow.h"
#include "InterSpec/PeakModel.h"
#include "InterSpec/HelpSystem.h"
#include "InterSpec/DetectorEdit.h"
#include "InterSpec/PhysicalUnits.h"
#include "InterSpec/SpecMeasManager.h"
#include "InterSpec/SpectraFileModel.h"
#include "SpecUtils/UtilityFunctions.h"
#include "InterSpec/RowStretchTreeView.h"

using namespace std;
using namespace Wt;

namespace FluxToolImp
{
  struct index_compare_sort
  {
    const std::vector<std::array<double,FluxToolWidget::FluxColumns::FluxNumColumns>> &arr;
    int m_sortColumn;
    Wt::SortOrder m_sortOrder;
    
    index_compare_sort(const std::vector<std::array<double,FluxToolWidget::FluxColumns::FluxNumColumns>> &arr, int col, Wt::SortOrder order)
      : arr(arr),
        m_sortColumn( col ),
        m_sortOrder( order )
    {
    }
    
    bool operator()(const size_t a, const size_t b) const
    {
      const bool less = arr[a][m_sortColumn] < arr[b][m_sortColumn];
      return (m_sortOrder==Wt::SortOrder::AscendingOrder ? less : !less);
    }
  };//struct index_compare
  
  
  class FluxModel : public  Wt::WAbstractItemModel
  {
  protected:
    FluxToolWidget *m_fluxtool;
    FluxToolWidget::FluxColumns m_sortColumn;
    Wt::SortOrder m_sortOrder;
    
    //Depending on sort column and order, we will map the rows of
    //  m_fluxtool->m_data to the row in this model
    vector<size_t> m_sort_indices;
    
  public:
    FluxModel( FluxToolWidget *parent )
    : WAbstractItemModel( parent ),
      m_fluxtool( parent ),
      m_sortColumn( FluxToolWidget::FluxColumns::FluxEnergyCol ),
      m_sortOrder( Wt::SortOrder::AscendingOrder )
    {
      parent->tableUpdated().connect( this, &FluxModel::handleFluxToolWidgetUpdated );
    }
    
    
    virtual ~FluxModel()
    {
    }
    
    
    virtual Wt::WFlags<Wt::ItemFlag> flags( const Wt::WModelIndex &index ) const
    {
      return Wt::WFlags<Wt::ItemFlag>(Wt::ItemFlag::ItemIsXHTMLText);
    }
    
    
    virtual WFlags<Wt::HeaderFlag> headerFlags( int section, Wt::Orientation orientation = Wt::Horizontal ) const
    {
      return Wt::WFlags<Wt::HeaderFlag>(Wt::HeaderFlag::HeaderIsXHTMLText);
    }
    
    
    void handleFluxToolWidgetUpdated()
    {
      if( !m_sort_indices.empty() )
      {
        beginRemoveRows( WModelIndex(), 0, static_cast<int>(m_sort_indices.size() - 1) );
        m_sort_indices.clear();
        endRemoveRows();
      }
      
      if( !m_fluxtool->m_data.empty() )
      {
        beginInsertRows( WModelIndex(), 0, static_cast<int>(m_fluxtool->m_data.size() - 1) );
        doSortWork();
        endInsertRows();
      }//
    }//void handleFluxToolWidgetUpdated()
    
    
    virtual boost::any data( const Wt::WModelIndex &index,
                             int role = Wt::DisplayRole ) const
    {
      if( index.parent().isValid() )
        return boost::any();
      
      const int row = index.row();
      const int col = index.column();
      
      if( row < 0 || col < 0
         || col >= FluxToolWidget::FluxColumns::FluxNumColumns
         || row >= static_cast<int>(m_sort_indices.size()) )
        return boost::any();
      
      const size_t realind = m_sort_indices[row];
      
      const auto &data = m_fluxtool->m_data;
      const auto &uncerts = m_fluxtool->m_uncertainties;
      
      assert( realind < data.size() );
      
      switch( role )
      {
        case Wt::DisplayRole:
          return boost::any( data[realind][col] );
          
        //case Wt::StyleClassRole: break;
        //case Wt::ToolTipRole: break;
        case Wt::UserRole:
          return uncerts[realind][col] > std::numeric_limits<double>::epsilon() ? boost::any(uncerts[realind][col]) : boost::any();

        default:
          return boost::any();
      }//switch( role )
      
      return boost::any();
    }//data(...)
    
    
    boost::any headerData( int section,
                          Wt::Orientation orientation = Wt::Horizontal,
                          int role = Wt::DisplayRole ) const
    {
      if( section < 0 || section >= FluxToolWidget::FluxColumns::FluxNumColumns
         || orientation != Wt::Horizontal
         || role != Wt::DisplayRole )
        return boost::any();
      return boost::any( m_fluxtool->m_colnames[section] );
    }
    
    virtual int columnCount( const Wt::WModelIndex &parent
                            = Wt::WModelIndex() ) const
    {
      if( parent.isValid() )
        return 0;
      return FluxToolWidget::FluxColumns::FluxNumColumns;
    }
    
    virtual int rowCount( const Wt::WModelIndex &parent = Wt::WModelIndex() ) const
    {
      if( parent.isValid() )
        return 0;
      return static_cast<int>( m_sort_indices.size() );
    }
    
    virtual Wt::WModelIndex parent( const Wt::WModelIndex &index) const
    {
      return WModelIndex();
    }
    
    virtual Wt::WModelIndex index( int row, int column,
                                  const Wt::WModelIndex &parent = Wt::WModelIndex() ) const
    {
      return WAbstractItemModel::createIndex(row, column, nullptr);
    }
    
    void doSortWork()
    {
      m_sort_indices.resize( m_fluxtool->m_data.size() );
      
      for( size_t i = 0; i < m_sort_indices.size(); ++i )
        m_sort_indices[i] = i;
      
      std::stable_sort( m_sort_indices.begin(), m_sort_indices.end(),
                       index_compare_sort(m_fluxtool->m_data,m_sortColumn,m_sortOrder) );
    }//void doSortWork()
    
    virtual void sort( int column, Wt::SortOrder order = Wt::AscendingOrder )
    {
      m_sortOrder = order;
      m_sortColumn = FluxToolWidget::FluxColumns( column );
      
      layoutAboutToBeChanged().emit();
      doSortWork();
      layoutChanged().emit();
    }//sort(...)
  };//class FluxModel(...)
  
  
  class FluxRenderDelegate : public WAbstractItemDelegate
  {
  public:
    FluxRenderDelegate( FluxToolWidget *parent )
    : WAbstractItemDelegate( parent )
    {
    }
    
    virtual WWidget *update(WWidget *widget, const WModelIndex& index,
                            WFlags<ViewItemRenderFlag> flags)
    {
      auto model = index.model();
      if( !model )
        return nullptr;
      auto data = index.data();
      auto uncertdata = model->data( index.row(), index.column(), Wt::UserRole );
      
      if( data.empty() )
        return nullptr;
      
      const bool hasUncert = !uncertdata.empty();
      
      double val = 0.0, uncert = 0.0;
      try
      {
        val = boost::any_cast<double>(data);
        if( hasUncert )
          uncert = boost::any_cast<double>(uncertdata);
      }catch(...)
      {
        return nullptr;
      }
      
      auto oldwidget = dynamic_cast<WText *>( widget );
      
      char buffer[128];
      
      if( index.column() == FluxToolWidget::FluxColumns::FluxEnergyCol )
      {
        snprintf( buffer, sizeof(buffer), "%.2f", val );
      }else
      {
        if( hasUncert )
          snprintf( buffer, sizeof(buffer), "%.4g &plusmn; %.1f", val, uncert );
        else
          snprintf( buffer, sizeof(buffer), "%.4g", val );
      }
      
      
      if( !oldwidget )
      {
        if( widget )
          delete widget;
        oldwidget = new WText();
      }
      
      oldwidget->setText( WString::fromUTF8(buffer) );
      return oldwidget;
    }
  };
  
  class FluxCsvResource : public Wt::WResource
  {
  protected:
    FluxToolWidget *m_fluxtool;
    
  public:
    FluxCsvResource( FluxToolWidget *parent )
    : WResource( parent ),
      m_fluxtool( parent )
    {
    }
    
    virtual ~FluxCsvResource()
    {
      beingDeleted();
    }
    
    
  private:
    virtual void handleRequest( const Wt::Http::Request &, Wt::Http::Response &response )
    {
      const string eol_char = "\r\n"; //for windows - could potentially cosutomize this for the users operating system
      
      if( !m_fluxtool )
        return;
      
      string filename;
      auto meas = m_fluxtool->m_interspec->measurment(SpectrumType::kForeground);
      if( meas && !meas->filename().empty() )
      {
        filename = UtilityFunctions::filename( meas->filename() );
        const string extension = UtilityFunctions::file_extension( filename );
        filename = filename.substr( 0, filename.size() - extension.size() );
        if( filename.size() )
          filename += "_";
      }
      filename += "flux.csv";
      
      
      suggestFileName( filename, WResource::Attachment );
      
      for( FluxToolWidget::FluxColumns col = FluxToolWidget::FluxColumns(0);
          col < FluxToolWidget::FluxColumns::FluxNumColumns; col = FluxToolWidget::FluxColumns(col+1) )
      {
        const WString &colname = m_fluxtool->m_colnamesCsv[col];
        
        response.out() << (col==0 ? "" : ",") << colname;
        
        //No uncertainty on energy.
        switch( col )
        {
          case FluxToolWidget::FluxEnergyCol:
          case FluxToolWidget::FluxIntrinsicEffCol:
          case FluxToolWidget::FluxGeometricEffCol:
          case FluxToolWidget::FluxNumColumns:
            break;
            
          case FluxToolWidget::FluxPeakCpsCol:
          case FluxToolWidget::FluxFluxOnDetCol:
          case FluxToolWidget::FluxFluxPerCm2PerSCol:
          case FluxToolWidget::FluxGammasInto4PiCol:
            response.out() << "," << (colname + " Uncertainty");
            break;
        }//switch( col )
      }//for( loop over columns )
      
      response.out() << eol_char;
      
      for( size_t row = 0; row < m_fluxtool->m_data.size(); ++row )
      {
        for( FluxToolWidget::FluxColumns col = FluxToolWidget::FluxColumns(0);
            col < FluxToolWidget::FluxColumns::FluxNumColumns; col = FluxToolWidget::FluxColumns(col+1) )
        {
          const double data = m_fluxtool->m_data[row][col];
          const double uncert = m_fluxtool->m_uncertainties[row][col];
          
          response.out() << (col==0 ? "" : ",") << std::to_string(data);
          switch( col )
          {
            case FluxToolWidget::FluxEnergyCol:
            case FluxToolWidget::FluxIntrinsicEffCol:
            case FluxToolWidget::FluxGeometricEffCol:
            case FluxToolWidget::FluxNumColumns:
              break;
              
            case FluxToolWidget::FluxPeakCpsCol:
            case FluxToolWidget::FluxFluxOnDetCol:
            case FluxToolWidget::FluxFluxPerCm2PerSCol:
            case FluxToolWidget::FluxGammasInto4PiCol:
              response.out() << ",";
              if( uncert > std::numeric_limits<double>::epsilon() )
                response.out() << std::to_string(uncert);
              break;
          }//switch( col )
        }//for( loop over columns )
        
        response.out() << eol_char;
      }//for( size_t row = 0; row < m_fluxtool->m_data.size(); ++row )
    }//handleRequest(...)
    
  };//class class FluxCsvResource
}//namespace


FluxToolWindow::FluxToolWindow( InterSpec *viewer )
: AuxWindow( "Flux Tool",
  (Wt::WFlags<AuxWindowProperties>(AuxWindowProperties::TabletModal)
   | AuxWindowProperties::SetCloseable
   | AuxWindowProperties::DisableCollapse) ),
  m_fluxTool( nullptr )
{
  assert( viewer );
  rejectWhenEscapePressed( true );
  
  m_fluxTool = new FluxToolWidget( viewer );
  //m_fluxTool->setHeight( WLength(100, WLength::Percentage) );
  
  stretcher()->addWidget( m_fluxTool, 0, 0 );
  
  AuxWindow::addHelpInFooter( footer(), "flux-tool" );
  
  
  WContainerWidget *buttonDiv = footer();
  
  WResource *csv = new FluxToolImp::FluxCsvResource( m_fluxTool );
#if( BUILD_AS_OSX_APP )
  WAnchor *csvButton = new WAnchor( WLink(csv), buttonDiv );
  csvButton->setTarget( AnchorTarget::TargetNewWindow );
#else
  WPushButton *csvButton = new WPushButton( buttonDiv );
  csvButton->setIcon( "InterSpec_resources/images/download_small.png" );
  csvButton->setLink( WLink(csv) );
  csvButton->setLinkTarget( Wt::TargetNewWindow );
#endif
  
  csvButton->setText( "CSV" );
  csvButton->setStyleClass( "CsvLinkBtn" );
  
  auto enableDisableCsv = [csvButton,this](){
    csvButton->setEnabled( !m_fluxTool->m_data.empty() );
  };
  
  m_fluxTool->m_tableUpdated.connect( std::bind(enableDisableCsv) );
  csvButton->disable();
  
  
  WPushButton *closeButton = addCloseButtonToFooter();
  closeButton->clicked().connect( this, &AuxWindow::hide );
  
  finished().connect( boost::bind( &AuxWindow::deleteAuxWindow, this ) );
  
  show();
  
  const int screenW = viewer->renderedWidth();
  const int screenH = viewer->renderedHeight();
  const int width = ((screenW < 680) ? screenW : 680);
  const int height = ((screenH < 420) ? screenH : 420);
  resizeWindow( width, height );
  
  resizeToFitOnScreen();
  centerWindowHeavyHanded();
}//FluxToolWindow(...) constrctor


FluxToolWindow::~FluxToolWindow()
{
}


FluxToolWidget::FluxToolWidget( InterSpec *viewer, Wt::WContainerWidget *parent )
  : WContainerWidget( parent ),
    m_interspec( viewer ),
    m_detector( nullptr ),
    m_msg( nullptr ),
    m_distance( nullptr ),
    m_table( nullptr ),
    m_needsTableRefresh( true ),
    m_compactColumns( false ),
    m_tableUpdated( this )
{
  init();
}


FluxToolWidget::~FluxToolWidget()
{
}


Wt::Signal<> &FluxToolWidget::tableUpdated()
{
  return m_tableUpdated;
}


void FluxToolWidget::init()
{
  assert( m_interspec );
  assert( !m_detector );
  
  for( FluxColumns col = FluxColumns(0); col < FluxColumns::FluxNumColumns; col = FluxColumns(col + 1) )
  {
    switch( col )
    {
      case FluxEnergyCol:
        m_colnames[col] = WString::fromUTF8("Energy (keV)");
        m_colnamesCsv[col] = WString::fromUTF8("Energy (keV)");
        break;
      case FluxPeakCpsCol:
        m_colnames[col] = WString::fromUTF8("Peak CPS");
        m_colnamesCsv[col] = WString::fromUTF8("Peak CPS");
        break;
      case FluxIntrinsicEffCol:
        m_colnames[col] = WString::fromUTF8("Intr. Eff.");
        m_colnamesCsv[col] = WString::fromUTF8("Intrinsic Efficiency");
        break;
      case FluxGeometricEffCol:
        m_colnames[col] = WString::fromUTF8("Geom. Eff.");
        m_colnamesCsv[col] = WString::fromUTF8("Geometric Efficiency");
        break;
      case FluxFluxOnDetCol:
        m_colnames[col] = WString::fromUTF8("Flux on Det. (&gamma;/s)");
        m_colnamesCsv[col] = WString::fromUTF8("Flux on Detector (gammas/s)");
        break;
      case FluxFluxPerCm2PerSCol:
        m_colnames[col] = WString::fromUTF8("Flux (&gamma;/cm&sup2;/s)");
        m_colnamesCsv[col] = WString::fromUTF8("Flux (gammas/cm2/s)");
        break;
      case FluxGammasInto4PiCol:
        m_colnames[col] = WString::fromUTF8("&gamma;/4&pi;/s");
        m_colnamesCsv[col] = WString::fromUTF8("gammas/4pi/s");
        break;
      case FluxNumColumns:        break;
    }//switch( col )
  }//for( loop over columns )
  
  
  wApp->useStyleSheet( "InterSpec_resources/FluxTool.css" );
  
  const bool showToolTipInstantly = m_interspec ? InterSpecUser::preferenceValue<bool>( "ShowTooltips", m_interspec ) : false;
  
  addStyleClass( "FluxToolWidget" );
  
  WGridLayout *layout = new WGridLayout();
  setLayout( layout );
  
  WTable *distDetRow = new WTable();
  distDetRow->addStyleClass( "FluxDistMsgDetTbl" );
  layout->addWidget( distDetRow, 0, 0 );
  
  SpectraFileModel *specFileModel = m_interspec->fileManager()->model();
  m_detector = new DetectorDisplay( m_interspec, specFileModel );
  m_detector->addStyleClass( "FluxDet" );
  m_interspec->detectorChanged().connect( this, &FluxToolWidget::setTableNeedsUpdating );
  m_interspec->detectorModified().connect( this, &FluxToolWidget::setTableNeedsUpdating );
  distDetRow->elementAt(0,1)->addWidget( m_detector );
  
  auto distCell = distDetRow->elementAt(0,0);
  distCell->addStyleClass( "FluxDistCell" );
  WLabel *label = new WLabel( "Distance:", distCell );
  label->addStyleClass( "FluxDistLabel" );
  
  m_distance = new WLineEdit( "100 cm", distCell );
  m_distance->addStyleClass( "FluxDistanceEnter" );
  label->setBuddy(m_distance);
  WRegExpValidator *validator = new WRegExpValidator( PhysicalUnits::sm_distanceUnitOptionalRegex, this );
  validator->setFlags( Wt::MatchCaseInsensitive );
  m_distance->setValidator( validator );
  HelpSystem::attachToolTipOn( m_distance,
                              "Distance from center of source to face of detector. Number must be"
                              " followed by units; valid units are: meters, m, cm, mm, km, feet,"
                              " ft, ', in, inches, or \".  You may also add multiple distances,"
                              " such as '3ft 4in', or '3.6E-2 m 12 cm' which are equivalent to "
                              " 40inches and 15.6cm respectively.", showToolTipInstantly );
  m_distance->changed().connect( this, &FluxToolWidget::setTableNeedsUpdating );
  m_distance->enterPressed().connect( this, &FluxToolWidget::setTableNeedsUpdating );
  
  
  PeakModel *peakmodel = m_interspec->peakModel();
  assert( peakmodel );
  peakmodel->dataChanged().connect( this, &FluxToolWidget::setTableNeedsUpdating );
  peakmodel->rowsRemoved().connect( this, &FluxToolWidget::setTableNeedsUpdating );
  peakmodel->rowsInserted().connect( this, &FluxToolWidget::setTableNeedsUpdating );
  peakmodel->layoutChanged().connect( this, &FluxToolWidget::setTableNeedsUpdating );

  auto msgCell = distDetRow->elementAt(1,0);
  msgCell->setColumnSpan( 2 );
  msgCell->addStyleClass( "FluxMsgCell" );
  m_msg = new WText( "", Wt::XHTMLText, msgCell );
  m_msg->addStyleClass( "FluxMsg" );
  
  
  FluxToolImp::FluxModel *fluxmodel = new FluxToolImp::FluxModel( this );
  m_table = new RowStretchTreeView();
  m_table->setModel( fluxmodel );
  m_table->setRootIsDecorated( false );
  m_table->addStyleClass( "FluxTable" );
  m_table->setAlternatingRowColors( true );
  m_table->sortByColumn( FluxColumns::FluxEnergyCol, Wt::AscendingOrder );
  m_table->setSelectable( true );
  //m_table->setSelectionMode( Wt::SelectionMode::SingleSelection );
  
  FluxToolImp::FluxRenderDelegate *renderdel = new FluxToolImp::FluxRenderDelegate( this );
  m_table->setItemDelegate( renderdel );
  
  for( FluxColumns col = FluxColumns(0); col < FluxColumns::FluxNumColumns; col = FluxColumns(col + 1) )
    m_table->setSortingEnabled( col, (col!=FluxColumns::FluxGeometricEffCol) );
  
  layout->addWidget( m_table, 1, 0 );
  layout->setRowStretch( 1, 1 );
  
  WCheckBox *cb = new WCheckBox( "show more info" );
  cb->addStyleClass( "FluxMoreInfoCB" );
  cb->setChecked( false );
  cb->changed().connect( std::bind( [this,cb](){
    setMinimalColumnsOnly( !cb->isChecked() );
  }) );
  
  layout->addWidget( cb, 2, 0, AlignRight );
  

  for( FluxColumns col = FluxColumns(0); col < FluxColumns::FluxNumColumns; col = FluxColumns(col + 1) )
  {
    WLength length;
    switch( col )
    {
      case FluxEnergyCol:         length = WLength(7.5, WLength::FontEm); break;
      case FluxPeakCpsCol:        length = WLength(7.5, WLength::FontEm); break;
      case FluxIntrinsicEffCol:   length = WLength(7.5, WLength::FontEm); break;
      case FluxGeometricEffCol:   length = WLength(7.5, WLength::FontEm); break;
      case FluxFluxOnDetCol:      length = WLength(7.5, WLength::FontEm); break;
      case FluxFluxPerCm2PerSCol: length = WLength(9.0, WLength::FontEm); break;
      case FluxGammasInto4PiCol:  length = WLength(9.0, WLength::FontEm); break;
      case FluxNumColumns:        break;
    }//switch( col )
    
    m_table->setColumnWidth( col, length);
  }//for( loop over columns )
  
  m_compactColumns = false; //set false so setMinimalColumnsOnly(true) will do work
  setMinimalColumnsOnly( true );
}//void init()


void FluxToolWidget::render( Wt::WFlags<Wt::RenderFlag> flags )
{
  if( m_needsTableRefresh )
    refreshPeakTable();
  
  WContainerWidget::render( flags );
}//void render( Wt::WFlags<Wt::RenderFlag> flags );


void FluxToolWidget::setTableNeedsUpdating()
{
  m_needsTableRefresh = true;
  scheduleRender();
}//void setTableNeedsUpdating()


void FluxToolWidget::refreshPeakTable()
{
  PeakModel *peakmodel = m_interspec->peakModel();
  
  m_data.clear();
  m_uncertainties.clear();
  
  m_msg->setText( "" );
  
  float distance = static_cast<float>( 1.0*PhysicalUnits::meter );
  try
  {
    distance = static_cast<float>( PhysicalUnits::stringToDistance( m_distance->text().toUTF8() ) );
  }catch(...)
  {
    m_msg->setText( "Invalid Distance" );
    m_tableUpdated.emit();
    return;
  }
  
  auto det = m_detector->detector();
  if( !det || !det->isValid() )
  {
    m_msg->setText( "No Detector Response Function Chosen" );
    m_tableUpdated.emit();
    return;
  }
  
  auto spec = m_interspec->measurment(SpectrumType::kForeground);
  if( !spec )
  {
    m_msg->setText( "No foreground spectrum loaded" );
    m_tableUpdated.emit();
    return;
  }
  
  const float live_time = spec->gamma_live_time();
  if( live_time <= 0.0f )
  {
    m_msg->setText( "Invalid foregorund livetime" );
    m_tableUpdated.emit();
    return;
  }
  
  const vector<PeakDef> peaks = peakmodel->peakVec();
  
  const size_t npeaks = peaks.size();
  m_data.resize( npeaks );
  m_uncertainties.resize( npeaks );

  for( int i = 0; i < static_cast<int>(npeaks); ++i )
  {
    m_data[i].fill( 0.0 );
    m_uncertainties[i].fill( 0.0 );
    
    const PeakDef &peak = peaks[i];
    
    const double energy = peak.mean();
    
    const double amp = peak.peakArea();  //ToDO: make sure this works for non-Gaussian peaks
    const double ampUncert = peak.peakAreaUncert();
    const double cps = amp / live_time;
    const double cpsUncert = ampUncert / live_time;
    const double intrisic = det->intrinsicEfficiency(energy);
    const double geomEff = det->fractionalSolidAngle( det->detectorDiameter(), distance );
    const double totaleff = det->efficiency(energy, distance );
    
    m_data[i][FluxColumns::FluxEnergyCol] = energy;
    m_data[i][FluxColumns::FluxPeakCpsCol] = cps;
    m_uncertainties[i][FluxColumns::FluxPeakCpsCol] = cpsUncert;
    
    m_data[i][FluxColumns::FluxGeometricEffCol] = geomEff;
    m_data[i][FluxColumns::FluxIntrinsicEffCol] = intrisic;
    //ToDo: Check if there is an uncertainty on DRF, and if so include that.
    
    if( totaleff <= 0.0 || intrisic <= 0.0 )
    {
      m_data[i][FluxColumns::FluxFluxOnDetCol]      = std::numeric_limits<double>::infinity();
      m_data[i][FluxColumns::FluxFluxPerCm2PerSCol] = std::numeric_limits<double>::infinity();
      m_data[i][FluxColumns::FluxGammasInto4PiCol]  = std::numeric_limits<double>::infinity();
    }else
    {
      const double fluxOnDet = cps / intrisic;
      const double fluxOnDetUncert = cpsUncert / intrisic;
      
      //gammas into 4pi
      const double gammaInto4pi = cps / totaleff;
      const double gammaInto4piUncert = cpsUncert / totaleff;
      
      //Flux in g/cm2/s
      const double flux = gammaInto4pi / (4*M_PI*distance*distance);
      const double fluxUncert = gammaInto4piUncert / (4*M_PI*distance*distance);
      
      
      m_data[i][FluxColumns::FluxFluxOnDetCol] = fluxOnDet;
      m_uncertainties[i][FluxColumns::FluxFluxOnDetCol] = fluxOnDetUncert;
      
      m_data[i][FluxColumns::FluxFluxPerCm2PerSCol] = flux;
      m_uncertainties[i][FluxColumns::FluxFluxPerCm2PerSCol] = fluxUncert;
      
      m_data[i][FluxColumns::FluxGammasInto4PiCol] = gammaInto4pi;
      m_uncertainties[i][FluxColumns::FluxGammasInto4PiCol] = gammaInto4piUncert;
    }//if( eff > 0 ) / else
  }//for( const PeakDef &peak : peaks )
  
  m_tableUpdated.emit();
}//void refreshPeakTable()


void FluxToolWidget::setMinimalColumnsOnly( const bool minonly )
{
  if( minonly == m_compactColumns )
    return;
  
  m_compactColumns = minonly;
  
  for( FluxColumns col = FluxColumns(0); col < FluxColumns::FluxNumColumns; col = FluxColumns(col + 1) )
  {
    bool show = true;
    switch( col )
    {
      case FluxEnergyCol:
      case FluxPeakCpsCol:
      case FluxFluxPerCm2PerSCol:
      case FluxGammasInto4PiCol:
        show = true;
      break;
        
      case FluxIntrinsicEffCol:
      case FluxGeometricEffCol:
      case FluxFluxOnDetCol:
        show = !m_compactColumns;
        break;
        
      case FluxNumColumns:
        break;
    }//switch( col )
    
    m_table->setColumnHidden( col, !show );
  }//for( loop over columns )
  
  m_table->refreshColWidthLayout();
}//void setMinimalColumnsOnly( const bool minonly )



