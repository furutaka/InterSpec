// To run:
//  npm install html-pdf
//  node help_topic_to_pdf.js

var fs = require('fs');
const path = require('path');
var pdf = require('html-pdf');

var origdir = process.cwd();
//var helpfile = "energy_calibration_help.xml";
//var helpfile = "compact_file_manager_help.xml";
var helpfile = "peak_manager_help.xml";

try{
  process.chdir('../../..');
console.log( 'cwd=' + process.cwd() );
  var html = fs.readFileSync( path.join('./InterSpec_resources/static_text/',helpfile), 'utf8' );

  html = '<!DOCTYPE html><html lang="en"><head><meta charset="utf-8"><title>title</title>'
         + '<link rel="stylesheet" href="InterSpec_resources/HelpWindow.css">'
         + '<link rel="stylesheet" href="InterSpec_resources/InterSpec.css">' 
         + '<link rel="stylesheet" href="build_xcode/resources/style.css">' 
         + '<link rel="stylesheet" href="build_xcode/resources/form.css">' 
         + '<link rel="stylesheet" href="build_xcode/resources/themes/default/wt.css">' 
         + '</head><body>'
         + html
         + '</body></html>'


  var options = { //format: 'A4',
                 "width": "10.5in",
                 "height": "65in",
                 
                 "base": "file://" + process.cwd() + '/' };
  var outputname =  path.join(origdir, helpfile);
  outputname = outputname.substring(0,outputname.length-3) + "pdf";

  pdf.create(html, options).toFile( outputname, function(err, res) {
    if( err ) 
      return console.log(err);
    console.log(res); 
  });
}catch(err){
  console.log(err);
}

process.chdir(origdir);
