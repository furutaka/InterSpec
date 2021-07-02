#ifndef QLSpectrumChart_h
#define QLSpectrumChart_h

/* QLSpectrumChart is a version of InterSpecs SpectrumChart class that was
   branched 20171227 to create a version slightly more acceptable for QuickLook
   functionality.
 */

#include <vector>
#include <memory>
#include <utility>


#include <Wt/WColor>
#include <Wt/WRectF>
#include <Wt/WLength>
#include <Wt/WEvent>
#include <Wt/WPainter>
#include <Wt/WAnimation>
#include <Wt/WPaintDevice>
#include <Wt/Chart/WCartesianChart>

#include "QLPeakDef.h"
#include "QLSpectrumDataModel.h"

//Forward declarations
namespace Wt
{
  class WContainerWidget;
}//namespace Wt

class AuxWindow;

namespace SpecUtils{ class Measurement; }


//See comments about QLSpectrumChart::setLeftYAxisPadding() for what
//  DYNAMICALLY_ADJUST_LEFT_CHART_PADDING controls.  This feature has not
//  been tested well enough to fully use yet, although it seems to work well.
#define DYNAMICALLY_ADJUST_LEFT_CHART_PADDING 1

class SeriesRenderer;
class LabelRenderIterator;
class MarkerRenderIterator;
class SpectrumSeriesRenderer;
class SpectrumRenderIterator;


class QLSpectrumChart : public Wt::Chart::WCartesianChart
{
public:
  enum DragAction
  {
    ZoomIn,
    HighLight,
    NoAction
  };//enum DragAction

  enum XAxisUnits
  {
    kkeV,
    kSeconds,
    kUndefinedUnits
  };//enum XAxisUnits

  enum HighlightRegionType
  {
    ForegroundHighlight,
    BackgroundHighlight,
    SecondForegroundHighlight
  };//enum HighlightRegionType
  
  
  //HighlightRegion is what is used to overlay a color over the chart, and are
  //  used for roughly two different usages:
  //    1) On the time history chart, show which time segments are used to sum
  //       for the foreground, background, and secondary spectrums.
  //       These regions are set and cleared by calling:
  //       setTimeHighLightRegions(...), and clearTimeHighlightRegions(...).
  //    2) Used by other tools to highlight an energy range on the spectrum
  //       chart.  These regions are added and removed by
  //       addDecorativeHighlightRegion(...), and
  //       removeDecorativeHighlightRegion(...).  When a region is added, a
  //       unique ID is returned, so that region can be removed, without
  //       effecting other decorative regions.
  struct HighlightRegion
  {
    float lowerx, upperx;
    size_t hash;  /* Hash values of 0, 1, and 2 are reserved for to indicate
                     ForegroundHighlight, BackgroundHighlight, or 
                     SecondForegroundHighlight, respectively*/
    Wt::WColor color;
  };//HighlightRegion
  
  
  enum PeakLabels
  {
    kShowPeakUserLabel,
    kShowPeakEnergyLabel,
    kShowPeakNuclideLabel,
    kShowPeakNuclideEnergies,
    kNumPeakLabels
  };//enum PeakLabels
  
public:
  QLSpectrumChart( Wt::WContainerWidget *parent = NULL );
  virtual ~QLSpectrumChart();

  void setPeaks( std::shared_ptr<const std::deque< std::shared_ptr<const QLPeakDef> > > peaks );
  
  //graphableAreaWidth(): the width of the actual chart area, in pixels
//  int graphableAreaWidth() const;
  
  //Note: do not overide height() and width() or else when used in a stretching
  //      layout side will get really messed up after the first painting
  Wt::WLength paintedWidth() const;
  Wt::WLength paintedHeight() const;
  
  //setTextInMiddleOfChart(...): draws some large text over the middle of the
  //  chart - used int the spectrum quizzer for text based questions
  void setTextInMiddleOfChart( const Wt::WString &txt );
  
  //textInMiddleOfChart(): returns current m_textInMiddleOfChart
  const Wt::WString &textInMiddleOfChart() const;

  //setCompactAxis(): whether to slim down axis for small displays (e.g. on
  //  phone).  Note that effects wont be seen until next time chart is rendered.
  //  You should also adjust padding axis title text appropriately; x-axis
  //  padding of 23px seems to be a reasonable value.
  //  Default is to not have compact axis.
  //Currently only effects x-axis.
  void setCompactAxis( const bool compact );
  bool isAxisCompacted() const;

  
  void showGridLines( const bool draw ); //shows horizantal and vertical
  void showVerticalLines( const bool draw );
  void showHorizontalLines( const bool draw );
  bool verticalLinesShowing() const;  // Added by christian (20170425)
  bool horizontalLinesShowing() const;
  
  void showHistogramIntegralsInLegend( const bool show = true );

  void setPeakLabelColor( PeakLabels label, uint32_t red, uint32_t green,
                          uint32_t blue, uint32_t alpha = 255 );
  void setShowPeakLabel( PeakLabels label, bool show );
  bool showingPeakLabel( PeakLabels label ) const;
  
  virtual void paint( Wt::WPainter& painter,
                     const Wt::WRectF& rectangle = Wt::WRectF() ) const;
  virtual void paintEvent( Wt::WPaintDevice *paintDevice );
  

  //paintOnChartLegend(): only paints if m_legentType==OnChartLegen
  virtual void paintOnChartLegend( Wt::WPainter &painter ) const;
  void paintTextInMiddleOfChart( Wt::WPainter &painter ) const;

  virtual void paintPeaks( Wt::WPainter& painter ) const;
  virtual void paintNonGausPeak( const QLPeakDef &peak, Wt::WPainter &painter ) const;

  virtual void drawPeakText( const QLPeakDef &peak, Wt::WPainter &painter,
                             Wt::WPointF peakMaxInPx ) const;
  
  
  //peakYVal(...): returns the y-value to paint for the center of the bin, for
  //  given the peak; includes the backgorund value
  static double peakYVal( const int bin, const QLPeakDef &peak,
                          const QLSpectrumDataModel *th1Model,
                          std::shared_ptr<const QLPeakDef> prevPeak,
                          std::shared_ptr<const QLPeakDef> nextPeak );
  
  //gausPeakBinValue(...): gets the x and y values for the marker, for bin
  //  of data specified.
  //May throw exception
  void gausPeakBinValue( double &xvalue, double &yvalue,
                         const QLPeakDef &peak, const int bin,
                         const QLSpectrumDataModel *th1Model,
                         const double axisMinX, const double axisMaxX,
                         const double axisMinY, const double axisMaxY,
                         std::shared_ptr<const QLPeakDef> prevPeak,
                         std::shared_ptr<const QLPeakDef> nextPeak ) const;

  //peakBackgroundVal(...): gives the bottom of the peak (e.g. the continuum)
  //  to paint.  Acounts for data background subtraction as well.
  static double peakBackgroundVal( const int bin,
                                   const QLPeakDef &peak,
                                   const QLSpectrumDataModel *th1Model,
                                   std::shared_ptr<const QLPeakDef> prevPeak,
                                   std::shared_ptr<const QLPeakDef> nextPeak );

  
  typedef std::shared_ptr<const QLPeakDef> PeadDefPtr;
  typedef std::vector<PeadDefPtr > PeadDefPtrVec;
  typedef PeadDefPtrVec::const_iterator PeakDefPtrVecIter;
  //next_peaks(...): hmmm, assumes all pointers are valid
  static PeakDefPtrVecIter next_peaks( PeakDefPtrVecIter peak_start,
                                       PeakDefPtrVecIter peak_end,
                                       PeadDefPtrVec &peaks,
                                       std::shared_ptr<const SpecUtils::Measurement> data );

  //paintGausPeak(): Paints peaks which are in a connected region.
  virtual void paintGausPeak( const std::vector<std::shared_ptr<const QLPeakDef> > &peaks,
                              Wt::WPainter &painter ) const;

  //drawIndependantGausPeak(...): draw a peak, not taking into account any other
  //  peaks.  If doFill==false then only the outline for the peak is drawn, or
  //  else the fill area of the peak will be drawn.
  void drawIndependantGausPeak( const QLPeakDef &peak,
                                Wt::WPainter& painter,
                                const bool doFill = false,
                                const double xstart = -999.9,
                                const double xend = -999.9 ) const;
  
  void visibleRange( double &xmin, double &xmax,
                     double &ymin, double &ymax ) const;

  float xUnitsPerPixel() const;
  
  //gausPeakDrawRange(...): returns if the peak is visible at all
  bool gausPeakDrawRange( int &minbin, int &maxbin,
                          const QLPeakDef &peak,
                          const double axisMinX,
                          const double axisMaxX ) const;
  
  virtual void paintHighlightRegions( Wt::WPainter& painter ) const;


  virtual void setYAxisScale( Wt::Chart::AxisScale scale );
  virtual void setXAxisRange( const double x1, const double x2 );
  virtual void setYAxisRange( const double y1, const double y2 );
  virtual void yAxisRangeFromXRange( const double x0, const double x1,
                                     double &y0, double &y1 );
  virtual void setAutoXAxisRange();  //sets x-range to full range
  virtual void setAutoYAxisRange();  //sets y-range, from current x-range,
                                     //  note that if the rebin factor of the
                                     //  QLSpectrumDataModel is updated, or the data is
                                     //  changed, you must re-call this funtion

  virtual void setXAxisUnits( XAxisUnits units );
  virtual XAxisUnits xAxisUnits() const;

  //Added for QuickLook
  void setShowYLabels( bool show );

  //Added for QuickLook
  void setPaintOnLegendFontSize( int size );
  
  //Adding or clearing the highlight regions does not cause an update to the
  //  chart; you need to manually call update() to cause a rerender.
  void clearTimeHighlightRegions( const HighlightRegionType region );
  void setTimeHighLightRegions( const std::vector< std::pair<double,double> > &p,
                            const HighlightRegionType type ); //does not refresh the chart
  
  //Decorative highlights get assigned an ID, so this way you can only remove
  //  a decorative highlight, if you know its ID.  This is an attempt to allow
  //  multiple widgets to have multiple regions that they can manage.
  //
  //removeDecorativeHighlightRegion(): returns if the removal was successful.
  //  If regionid is less than 3, no action will be taken.
  bool removeDecorativeHighlightRegion( size_t regionid );
  
  //addDecorativeHighlightRegion(): returns the ID for the highlihgted region
  //  added; you will need to retain this in order to remove the region in the
  //  future.  Returned ID will alway be greater than 2.
  size_t addDecorativeHighlightRegion( const float lowerx,
                                       const float upperx,
                                       const Wt::WColor &color );

  void setDragAction( const DragAction action );


  //enableLegend():  Makes it so the legend will be rendered, with some caviots.
  //  If no spectrums are present, will not be rendered.
  //  If a time series chart (DragAction==Highlight), only rendered if more than
  //  one sereies is to be plotted.
  //  For non-phone devices, creates a globablly floating legend (AuxWindow)
  //  that will be kept at a consistent distance from the top and right hand
  //  side of chart.
  //  For phone devices, the legend will be rendered directly on the chart.
  void enableLegend( const bool forceMobileStyle );
  
  //disableLegend(): causes legend to stop being drawn.
  void disableLegend();
  
  bool legendIsEnabled() const;
  
  Wt::Signal<> &legendEnabled();
  Wt::Signal<> &legendDisabled();

  //legendExpandedCallback(): rerenders the legend after its been expanded
  void legendExpandedCallback();
  
  void legendTextSizeCallback( const float legendwidth );
  
  virtual void setHidden( bool hidden,
                          const Wt::WAnimation &animation = Wt::WAnimation() );

#if(DYNAMICALLY_ADJUST_LEFT_CHART_PADDING)
  //setLeftYAxisPadding(): tries to guess how many characters will be used for
  //  the y-axis label, and then sets the left padding of the chart accordingly,
  //  updating the client side mouse coordinate equations as well.
  //  The width and height are as given by the WPaintDevice the chart will be
  //  painted which is usually equivalent to
  //    width = chartArea().width() + plotAreaPadding(Left) + plotAreaPadding(Right);
  //    height = chartArea().height() + plotAreaPadding(Top) + plotAreaPadding(Bottom);
  //  Returns -1 on error, 0 if no adjustment was necassary, and 1 if adjustment
  //  was made.
  int setLeftYAxisPadding( double width, double height );
  
  //QuickLook customization: next two functions added 
  int setRightYAxisPadding( double width, double height );
  int setYAxisPadding( double width, double height, bool left = true );
#endif

protected:
  //modelChanged(): hooks up the signals to catch when the legend needs to be
  //  re-rendered.  Also enforces that only QLSpectrumDataModel models can be
  //  used with this class, by throwing an exception otherwise.
  virtual void modelChanged();
  
  
  void setLegendNeedsRendered();
  
protected:

  void labelRender( Wt::WPainter& painter, const Wt::WString& text,
                   const Wt::WPointF& p, const Wt::WColor& color,
                   Wt::WFlags<Wt::AlignmentFlag> flags,
                   double angle, int margin) const;
  
  virtual void render( Wt::WPainter &painter, const Wt::WRectF &rectangle ) const;
  
  virtual void renderBackground( Wt::WPainter &painter ) const;
  virtual void renderGridLines( Wt::WPainter &painter, const Wt::Chart::Axis axis ) const;
  virtual void renderSeries( Wt::WPainter &painter ) const;
  virtual void renderAxes( Wt::WPainter &painter, Wt::WFlags<  Wt::Chart::AxisProperty > properties ) const;
  virtual void renderXAxis( Wt::WPainter &painter, const Wt::Chart::WAxis& axis,
                            Wt::WFlags<Wt::Chart::AxisProperty> properties ) const;
  virtual void renderYAxis( Wt::WPainter &painter, const Wt::Chart::WAxis &axis,
                           Wt::WFlags<Wt::Chart::AxisProperty> properties ) const;
  virtual void renderLegend( Wt::WPainter &painter ) const;
  virtual bool isLargeEnough( Wt::WPainter &painter ) const;
  
  virtual void iterateSpectrum( SpectrumRenderIterator *iterator,
                                Wt::WPainter &painter ) const;
  
  //since we cant access WCartesianChar::chartArea_ ... (stupid Wt not allowing access)
  //  Note that providing a default WRectF() is a hack and will lead to
  //  rendering
  mutable Wt::WRectF m_renderRectangle;
  void calcRenderRectangle( const Wt::WRectF &rectangle ) const;
  const Wt::WRectF &chartArea() const;
  
  friend class SeriesRenderer;
  friend class LabelRenderIterator;
  friend class MarkerRenderIterator;
  friend class SpectrumRenderIterator;
  friend class SpectrumSeriesRenderer;
  
protected:
  double m_widthInPixels;        //The width of the chart in pixels
  double m_heightInPixels;       //The height of the chart in pixels
  
  
  //xRangeChanged() signal will be emitted whenever the user does a 'drag'
  //  action when m_dragAction==ZoomIn.
  //If m_dragAction==HighLight then the xRangeChanged() will only be emmitted
  //  if it was with the left mouse button, and there where no modifier keys.
  DragAction m_dragAction;
  

  std::shared_ptr<const std::deque< std::shared_ptr<const QLPeakDef> > > m_peaks;
  
  Wt::WColor m_peakFillColor;
  Wt::WColor m_multiPeakOutlineColor;
  
  //Potential ToDo: Right now m_highlights is mainly modified and added
  //  to by this class, however the effects of what is highlighted are also seen
  //  in the InterSpec class, which is a possibility for the displayed time
  //  range to get out of sync with the highlighted tiem ranges.  It might make
  //  sense for this class to never modify m_highlights, but instead
  //  just have the InterSpec class say which regions should be highlighted

  std::vector< HighlightRegion > m_highlights;
  
  
  enum LegendType
  {
    NoLegend,
    OnChartLegend
  };//enum LegendType
  
  
  //m_legendType: indicates the type of legend that should be rendered.  Decided
  //  in enableLegend() based on the device type.
  LegendType m_legendType;
  
  //QuickView customization
  int m_paintOnLegendFontSize;
  
  //m_legendNeedsRender: the legend doesnt need to be rendered everytime the
  //  chart is updated, but only when the data changes, so this variable tracks
  //  that.
  bool m_legendNeedsRender;
  
  //m_paintedLegendWidth: for m_legendType==OnChartLegend, with the chart being
  //  rendered to a <canvas> element, we dont have server side font metrics, so
  //  will execute some javascript to measure the font metrics, and then call
  //  back to legendTextSizeCallback(...) to set the following variable.
  float m_paintedLegendWidth;
  std::unique_ptr< Wt::JSignal<float> > m_legendTextMetric;
  
  //
  bool m_peakLabelsToShow[kNumPeakLabels];
  uint32_t m_peakLabelsColors[kNumPeakLabels]; //{red, green, blue, alpha}
  
  //if m_xAxisUnits==kUndefinedUnits, then mouse coordinates wont be displayed
  XAxisUnits m_xAxisUnits;
  
  
  bool m_compactAxis;
  bool m_showYAxisLabels;
  Wt::WString m_textInMiddleOfChart;
  
  // Added by christian (20170425)
  bool m_verticalLinesShowing;
  bool m_horizontalLinesShowing;
};//class QLSpectrumChart




 #endif
