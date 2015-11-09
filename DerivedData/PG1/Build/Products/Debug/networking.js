jQuery(function() {
  $(document).on('click', '#process', function() {
    if ($("#inputType").val() == "File"){
      $.ajax({
        url: '/process/file',
        method: 'POST',
        dataType: 'json',
        data: { 
          increment: $('#increment').val(),
          filename: $('#fajl').val()
        },
        success: function(json) {
          generateSongChart(json.song);    
          $("#dftSettings").show();
        },
        error: function(json) {
          console.log("error");
        },
      });
    }else{
      $.ajax({
        url: '/process/sine',
        method: 'POST',
        dataType: 'json',
        data: { 
          nSamples: $('#nSamples').val(),
          sines: $('#sines').val(),
        },
        success: function(json) {
          generateSongChart(json.song); 
          $("#dftSettings").show();
        },
        error: function(json) {
          console.log("error");
        },
      });
    }
  });

  $(document).on('click', '#dft', function() {
    $.ajax({
      url: '/dft',
      method: 'POST',
      dataType: 'json',
      data: { 
        windowWidth: $('#windowWidth').val(),
        windowFunction: $('#windowFunction').val(),
        windowOffset: $('#windowOffset').val(),
        average: $('#average').is(':checked')?"yes":"no",
      },
      success: function(json) {
        generateDftChart(json.song);
        generateSpectre(json.dft);
      },
      error: function(json) {
        console.log("error");
      },
    });
  });

});