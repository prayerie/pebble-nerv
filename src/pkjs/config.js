module.exports = [
    {
      "type": "heading",
      "defaultValue": "Watchface config"
    },
    {
      "type": "section",
      "items": [
        {
          "type": "heading",
          "defaultValue": "Colour scheme (next update)"
        },
        {
          "type": "radiogroup",
          "messageKey": "colourScheme",
          "label": "Colour scheme",
          "defaultValue": "d",
          "options": [
            { 
              "label": "Default", 
              "value": "d"
            },
            { 
              "label": "Rei", 
              "value": "r"
            },
            { 
              "label": "Asuka",
              "value": "a"
            },
            { 
              "label": "Misato", 
              "value": "m"
            }
          ]
        }
      ]
    },
    {
      "type": "section",
      "items": [
        {
          "type": "heading",
          "defaultValue": "Settings"
        },
        {
          "type": "toggle",
          "messageKey": "fahrenheit",
          "label": "Use Fahrenheit",
          "defaultValue": false
        },
        {
          "type": "toggle",
          "messageKey": "hideDotw",
          "label": "Hide day of the week dialogue",
          "defaultValue": false
        },
        {
          "type": "toggle",
          "messageKey": "japanese",
          "label": "Set some UI to Japanese",
          "defaultValue": false
        },
        {
          "type": "toggle",
          "messageKey": "hideTemp",
          "label": "Hide temperature module",
          "defaultValue": false
        }
      ]
    },
    {
      "type": "section",
      "items": [
        {
          "type": "input",
          "messageKey": "customKey",
          "defaultValue": "",
          "label": "Custom OpenWeatherMap API key",
          "attributes": {
            "placeholder": "Leave blank to use built-in key",
            "limit": 32,
          }
        },
        {
          "type": "input",
          "messageKey": "customLoc",
          "defaultValue": "",
          "label": "Override location",
          "attributes": {
            "placeholder": "Leave blank to use current GPS location",
            "limit": 32,
          }
        },
        {
          "type": "input",
          "messageKey": "secret",
          "defaultValue": "",
          "label": "?",
          "attributes": {
            "placeholder": "",
            "limit": 32,
          }
        }
      ]
    },
    {
      "type": "submit",
      "defaultValue": "Save"
    }
  ];