/* InterSpec: an application to analyze spectral gamma radiation data.
 
 Copyright 2018 National Technology & Engineering Solutions of Sandia, LLC
 (NTESS). Under the terms of Contract DE-NA0003525 with NTESS, the U.S.
 Government retains certain rights in this software.
 For questions contact William Johnson via email at wcjohns@sandia.gov, or
 alternative emails of interspec@sandia.gov.
 
 This library is free software; you can redistribute it and/or
 modify it under the terms of the GNU Lesser General Public
 License as published by the Free Software Foundation; either
 version 2.1 of the License, or (at your option) any later version.
 
 This library is distributed in the hope that it will be useful,
 but WITHOUT ANY WARRANTY; without even the implied warranty of
 MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
 Lesser General Public License for more details.
 
 You should have received a copy of the GNU Lesser General Public
 License along with this library; if not, write to the Free Software
 Foundation, Inc., 51 Franklin Street, Fifth Floor, Boston, MA  02110-1301  USA
 */

#include "InterSpec_config.h"

#include <Wt/Utils>
#include <Wt/WText>
#include <Wt/WImage>
#include <Wt/WLabel>
#include <Wt/WCheckBox>
#include <Wt/WPushButton>
#include <Wt/WApplication>
#include <Wt/WMemoryResource>
#include <Wt/WContainerWidget>

#include "SpecUtils/DateTime.h"
#include "SpecUtils/SpecFile.h"
#include "SpecUtils/StringAlgo.h"

#include "InterSpec/SpecMeas.h"
#include "InterSpec/InterSpec.h"
#include "InterSpec/SimpleDialog.h"
#include "InterSpec/InterSpecUser.h"
#include "InterSpec/MultimediaDisplay.h"

using namespace std;
using namespace Wt;

namespace
{

//Class to display a DetectorAnalysis; just a first go at it
//  Some sort of model and table, or something might be a better implementation.
class MultimediaDisplay : public WContainerWidget
{
  std::shared_ptr<const SpecMeas> m_meas;
  size_t m_current_index;
  WMemoryResource *m_resource;
  WImage *m_image;
  WText *m_error;
  WContainerWidget *m_add_info;
  WText *m_remark;
  WText *m_description;
  WText *m_time;
  WContainerWidget *m_nav;
  WPushButton *m_prev;
  WText *m_pos_txt;
  WPushButton *m_next;
#if( BUILD_AS_OSX_APP )
  WAnchor *m_download;
#else
  WPushButton *m_download;
#endif
  
public:
  MultimediaDisplay( WContainerWidget *parent = nullptr )
  : WContainerWidget( parent ),
  m_meas{ nullptr },
  m_current_index( 0 ),
  m_resource( nullptr ),
  m_image( nullptr ),
  m_error( nullptr ),
  m_add_info( nullptr ),
  m_remark( nullptr ),
  m_description( nullptr ),
  m_time( nullptr ),
  m_nav( nullptr ),
  m_prev( nullptr ),
  m_pos_txt( nullptr ),
  m_next( nullptr ),
  m_download( nullptr )
  {
    wApp->useStyleSheet( "InterSpec_resources/MultimediaDisplay.css" );
    
    addStyleClass( "MultimediaDisplay" );
  
    m_resource = new WMemoryResource( this );
    
    m_image = new WImage( WLink(m_resource), this );
    m_image->addStyleClass( "SpecImage" );
    m_image->setHidden( true );
    
    m_error = new WText( this );
    m_error->setStyleClass( "NoContentTxt" );
    
    m_add_info = new WContainerWidget( this );
    m_add_info->addStyleClass( "AddInfo" );
    
    m_remark = new WText( m_add_info );
    m_remark->addStyleClass( "Remark" );
    
    m_description = new WText( m_add_info );
    m_description->addStyleClass( "Description" );
    
    m_time = new WText( m_add_info );
    m_time->addStyleClass( "Time" );
    
    m_nav = new WContainerWidget( this );
    m_nav->addStyleClass( "Nav" );
    
    m_prev = new WPushButton( "Prev", m_nav );
    m_prev->addStyleClass( "Prev" );
    m_prev->clicked().connect( this, &MultimediaDisplay::prevIndex );
    
    m_pos_txt = new WText( m_nav );
    m_pos_txt->addStyleClass( "NavPos" );
    
    m_next = new WPushButton( "Next", m_nav );
    m_next->addStyleClass( "Next" );
    m_next->clicked().connect( this, &MultimediaDisplay::nextIndex );
    
    
    WContainerWidget *footer = new WContainerWidget( this );
    footer->addStyleClass( "PrefAndDownload" );
    
    WCheckBox *cb = new WCheckBox( "Auto show images", footer );
    cb->setToolTip( "Automatically show images " );
    m_next->setFocus();
    
    InterSpec *interspec = InterSpec::instance();
    assert( interspec );
    if( interspec )
      InterSpecUser::associateWidget( interspec->m_user, "AutoShowSpecMultimedia", cb, interspec );
    
#if( BUILD_AS_OSX_APP )
    m_download = new WAnchor( WLink(m_resource), footer );
    m_download->setTarget( AnchorTarget::TargetNewWindow );
    m_download->setStyleClass( "LinkBtn DownloadLink" );
    m_download->setTarget( AnchorTarget::TargetNewWindow ); //TargetDownload
    m_download->setText( "Save..." );
#else
    m_download = new WPushButton( footer );
    m_download->setIcon( "InterSpec_resources/images/download_small.svg" );
    m_download->setLink( WLink(m_resource) );
    m_download->setLinkTarget( Wt::TargetNewWindow ); //TargetDownload
    m_download->setStyleClass( "LinkBtn DownloadBtn" );
    
#if( ANDROID )
    // Using hacked saving to temporary file in Android, instead of via network download of file.
    m_download->clicked().connect( std::bind([downloadResource](){
      android_download_workaround(downloadResource, "image_from_spec_file");
    }) );
#endif //ANDROID
#endif
    
    m_download->setToolTip( "Save the currently showing image.", Wt::PlainText );
    
    setIndex( 0 );
  }//constructor
  
  void nextIndex()
  {
    if( !m_meas || (m_meas->multimedia_data().size() < 2) )
    {
      setIndex( 0 );
      return;
    }
    
    const size_t index = ((m_current_index + 1) % m_meas->multimedia_data().size());
    setIndex( index );
    m_next->setFocus();
  }//void nextIndex()
  
  
  void prevIndex()
  {
    if( !m_meas || (m_meas->multimedia_data().size() < 2) )
    {
      setIndex( 0 );
      return;
    }
    
    size_t index = m_current_index > 0 ? m_current_index - 1 : m_meas->multimedia_data().size() - 1;
    setIndex( index );
    m_prev->setFocus();
  }//void prevIndex()
  
  
  void setIndex( size_t index )
  {
    vector<shared_ptr<const SpecUtils::MultimediaData>> all_data;
    if( m_meas )
      all_data = m_meas->multimedia_data();
    
    auto showErrorMsg = [this]( const string &msg ){
      m_image->hide();
      m_resource->setData( {} );
      m_add_info->hide();
      m_nav->hide();
      m_error->show();
      m_error->setText( msg );
      m_download->hide();
      m_nav->setHidden( !m_meas || (m_meas->multimedia_data().size() < 2) );
    };//showErrorMsg lamda
    
    if( all_data.empty() )
    {
      showErrorMsg( "No multimedia content to display" );
      return;
    }//if( all_data.empty() )
  
  
    index = (index % all_data.size());
    m_current_index = index;
    
    shared_ptr<const SpecUtils::MultimediaData> data = all_data[index];
    
    if( !data || (data->data_.size() < 25) )
    {
      if( data && data->file_uri_.empty() )
        showErrorMsg( "Data encoded as a file at URI='" + data->file_uri_ + "'" );
      else
        showErrorMsg( "Multimedia source did not contain any data." );
      
      return;
    }//if( !data || (data->data_.size() < 25) )
    
    string data_str( begin(data->data_), end(data->data_) );
      
    switch( data->data_encoding_ )
    {
      case SpecUtils::MultimediaData::EncodingType::BinaryUTF8:
        data_str.clear();
        break;
          
      case SpecUtils::MultimediaData::EncodingType::BinaryHex:
        data_str = Wt::Utils::hexDecode(data_str);
        break;
          
      case SpecUtils::MultimediaData::EncodingType::BinaryBase64:
        data_str = Wt::Utils::base64Decode(data_str);
        break;
    }//switch( data->data_encoding_ )
      
    vector<unsigned char> data_uchar( (const unsigned char *)data_str.c_str(),
                                      (const unsigned char *)(data_str.c_str() + data_str.size()) );
      
    const string mime = Wt::Utils::guessImageMimeTypeData(data_uchar);
      
    if( data_str.empty() )
    {
      showErrorMsg( "Multimedia content encoding of data is not supported." );
      return;
    }
      
    if( mime.empty() )
    {
      showErrorMsg( "Multimedia content type was either not of an image type, or could not be determined." );
      return;
    }
      
    m_resource->setData( data_uchar );
    m_resource->setMimeType( mime );
    
    string filename = "image_" + to_string(m_current_index + 1) + "_of_" + to_string(all_data.size());
    
    if( SpecUtils::icontains(mime, "png") )
      filename += ".png";
    else if( SpecUtils::icontains(mime, "jpeg") )
      filename += ".jpeg";
    else if( SpecUtils::icontains(mime, "gif") )
      filename += ".gif";
    else if( SpecUtils::icontains(mime, "bmp") )
      filename += ".bmp";
    m_resource->suggestFileName( filename );
     
    m_image->show();
    m_error->hide();
    m_nav->setHidden( (all_data.size() < 2) );
    m_download->show();
    
    const string postxt = "Image " + to_string(m_current_index + 1)
                          + " of " + to_string(all_data.size());
    m_pos_txt->setText( postxt );
    
    
    bool have_add_info = false;
    if( SpecUtils::is_special( data->capture_start_time_ ) )
    {
      m_time->setText( "" );
      m_time->hide();
    }else
    {
      have_add_info = true;
      m_time->setText( "Time: " + SpecUtils::to_common_string(data->capture_start_time_, true) );
      m_time->show();
    }
    
    if( data->remark_.empty() )
    {
      m_remark->hide();
      m_remark->setText( "" );
    }else
    {
      have_add_info = true;
      m_remark->show();
      m_remark->setText( "Remark: " + data->remark_ );
    }
    
    if( data->descriptions_.empty() )
    {
      m_description->hide();
      m_description->setText( "" );
    }else
    {
      have_add_info = true;
      m_description->show();
      m_description->setText( "Desc: " + data->descriptions_ );
    }
    
    m_add_info->setHidden( !have_add_info );
    
    // Just in case the image changes the dialogs size, trigger it to resize.
    doJavaScript( "{let a = function(ms){"
                 + wApp->javaScriptClass() + ".layouts2.scheduleAdjust();"
                 " setTimeout( function(){ window.dispatchEvent(new Event('resize')); }, ms );"
                 "};"
                 "a(0); a(100); a(1000); a(5000);}" );
  }//void setIndex( size_t index )
  
  
  void updateDisplay( std::shared_ptr<const SpecMeas> meas )
  {
    m_meas = meas;
    m_current_index = 0;
    setIndex( m_current_index );
  }//void updateDisplay( std::shared_ptr<const SpecMeas> meas )

 
};//class AnaResultDisplay
}//namespace


SimpleDialog *displayMultimedia( const std::shared_ptr<const SpecMeas> &spec )
{
  wApp->useStyleSheet( "InterSpec_resources/MultimediaDisplay.css" );
  
  
  auto dialog = new SimpleDialog();
  //dialog->setModal( false ); //doesnt seem to have any effect
  dialog->addButton( "Close" );
  
  WContainerWidget *contents = dialog->contents();
  const bool multiple_images = (spec && (spec->multimedia_data().size() > 1));
  const char *title = multiple_images ? "Images in Spectrum File" : "Image in Spectrum File";
  WText *dialogTitle = new WText( title, contents );
  dialogTitle->addStyleClass( "title MultimediaDialogTitle" );
  dialogTitle->setInline( false );
  
  MultimediaDisplay *display = new MultimediaDisplay( contents );
  display->updateDisplay( spec );
  
  return dialog;
}//SimpleDialog *displayMultimedia( const std::shared_ptr<const SpecMeas> &spec )
