/* 
 Schema of discoveryConfig should be in the following format:  
    modelNumber:
        softwareVersion:
            <configuration>

The modelNumber and softwareVersion should match the values of the IoT Thing
attributes of the same name. 
*/

const discoveryConfig = {
    'smartThing-v1': {
        '1.00': {
            manufacturerName: 'SmartHome Products, Inc.',
            modelName: 'Model 001',
            friendlyName: 'Smart Device',
            description: 'My SmartHome Product!',
            displayCategories: ['LIGHT'],
            capabilities: [
                {
                    // Basic capability that should be included for all
                    // Alexa Smart Home API discovery responses:
                    type: 'AlexaInterface',
                    interface: 'Alexa',
                    version: '3'
                },
                {
                    type: "AlexaInterface",
                    interface: "Alexa.EndpointHealth",
                    "version":"3",
                    properties: {
                        supported: [
                          {
                             name: "connectivity"
                          }
                       ],
                       retrievable: true
                    }
                },
                {
                    type: "AlexaInterface",
                    interface: "Alexa.PowerController",
                    version: "3",
                    properties: {
                        supported: [
                            {
                                "name": "powerState"
                            }
                        ],
                     proactivelyReported: true,
                        retrievable: true
                    }
                },
              {
                    type: "AlexaInterface",
                    interface: "Alexa.BrightnessController",
                    version: "3",
                    properties: {
                        supported: [
                            {
                                "name": "color"
                            }
                        ],
                     proactivelyReported: true,
                        retrievable: true
                    }
                },
                {
                    type: "AlexaInterface",
                    interface: "Alexa.ColorController",
                    version: "3",
                    properties: {
                        supported: [
                            {
                                "name": "brightness"
                            }
                        ],
                     proactivelyReported: true,
                        retrievable: true
                    }
                }
            ]
        }
    } 
};

module.exports = discoveryConfig;
