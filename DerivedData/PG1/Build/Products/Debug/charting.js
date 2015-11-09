var schart;
var dchart;
var specchart;

$(document).ready(function() {
  changeSource(); 
  generateSongChart([]);
  generateDftChart([]);
  generateSpectre([]);
  AmCharts.checkEmptyData(schart);
  AmCharts.checkEmptyData(dchart);
  AmCharts.checkEmptyData(specchart);
  $('.modal-trigger').leanModal();
  $('select').material_select();
  $('#windowFunction').material_select();
  $("#dftSettings").hide();
});
$(document).on('change', '#average', function() {
  b = $('#average').is(':checked');
  if(b){
    $('#windowOffset').attr("disabled", 'disabled');
  }else{
    $('#windowOffset').removeAttr("disabled");
  }
});

function changeSource () {
  if ($("#inputType").val() == "File") {
    $("#sineSettings").hide();
    $("#fileSettings").show();
    $('label[for="windowWidth"]').html("Window width (ms)");
    $('label[for="windowOffset"]').html("Window offset (ms)");
  }else{
    $("#sineSettings").show();
    $("#fileSettings").hide();
    $('label[for="windowWidth"]').html("Window width (samples)");
    $('label[for="windowOffset"]').html("Window offset (samples)");
  }
}

function generateSongChart (chartData) {
  schart = AmCharts.makeChart("songchartdiv", {
    type: "serial",
    dataProvider: chartData,
    categoryField: "time",
    categoryAxis: {
      parseDates: false,
      gridAlpha: 0.15,
      minorGridEnabled: true,
      axisColor: "#DADADA",
    },
    valueAxes: [{
      axisAlpha: 0.2,
      id: "v1"
    }],
    graphs: [{
      title: "red line",
      id: "g1",
      valueAxis: "v1",
      valueField: "value",
      bullet: "none",
      bulletBorderColor: "#FFFFFF",
      bulletBorderAlpha: 1,
      lineThickness: 2,
      lineColor: "#b5030d",
      negativeLineColor: "#0352b5",
      balloonText: "[[category]]<br><b><span style='font-size:14px;'>value: [[value]]</span></b>"
    }],
    chartCursor: {
      fullWidth:true,
      cursorAlpha:0.1
    },
    chartScrollbar: {
      scrollbarHeight: 40,
      color: "#FFFFFF",
      autoGridCount: true,
      graph: "g1"
    },

    mouseWheelZoomEnabled:true
  });
  //schart.zoomToIndexes(0, chartData.length - 1);
}
function generateDftChart (chartData) {
  dchart = AmCharts.makeChart("dftchartdiv", {
    type: "serial",
    dataProvider: chartData,
    categoryField: "time",
    categoryAxis: {
      parseDates: false,
      gridAlpha: 0.15,
      minorGridEnabled: true,
      axisColor: "#DADADA",
    },
    valueAxes: [{
      axisAlpha: 0.2,
      id: "v1"
    }],
    graphs: [{
      title: "red line",
      id: "g1",
      valueAxis: "v1",
      valueField: "value",
      bullet: "none",
      bulletBorderColor: "#FFFFFF",
      bulletBorderAlpha: 1,
      lineThickness: 2,
      lineColor: "#b5030d",
      negativeLineColor: "#0352b5",
      balloonText: "[[category]]<br><b><span style='font-size:14px;'>value: [[value]]</span></b>"
    }],
    chartCursor: {
      fullWidth:true,
      cursorAlpha:0.1
    },
    chartScrollbar: {
      scrollbarHeight: 40,
      color: "#FFFFFF",
      autoGridCount: true,
      graph: "g1"
    },

    mouseWheelZoomEnabled:true
  });
}


AmCharts.checkEmptyData = function (chart) {
  if ( 0 == chart.dataProvider.length ) {
        // set min/max on the value axis
        chart.valueAxes[0].minimum = 0;
        chart.valueAxes[0].maximum = 100;
        
        // add dummy data point
        var dataPoint = {
          dummyValue: 0
        };
        dataPoint[chart.categoryField] = '';
        chart.dataProvider = [dataPoint];
        
        // add label
        chart.addLabel(0, '50%', 'The chart contains no data', 'center');
        
        // set opacity of the chart div
        chart.chartDiv.style.opacity = 0.5;
        
        // redraw it
        chart.validateNow();
      }
    }

    function generateSpectre (chartData) {

      specchart = new AmCharts.AmSerialChart();
      specchart.dataProvider = chartData;
      specchart.categoryField = "frequency";

      var categoryAxis = specchart.categoryAxis;
      categoryAxis.labelRotation = 90;
      categoryAxis.gridPosition = "start";

      var graph = new AmCharts.AmGraph();
      graph.valueField = "magnitude";
      graph.balloonText = "[[frequency]]: <b>[[magnitude]]</b>";
      graph.type = "column";
      graph.lineAlpha = 0;
      graph.fillAlphas = 0.8;
      specchart.addGraph(graph);

      var chartCursor = new AmCharts.ChartCursor();
      chartCursor.cursorAlpha = 0.5;
      chartCursor.zoomable = true;
      chartCursor.categoryBalloonEnabled = false;
      specchart.addChartCursor(chartCursor);

      specchart.creditsPosition = "top-left";

      specchart.write("spectrediv");
    }
