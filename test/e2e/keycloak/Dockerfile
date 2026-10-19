FROM quay.io/keycloak/keycloak:26.7.4

COPY uda-test-realm.json /opt/keycloak/data/import/uda-test-realm.json

EXPOSE 8080

CMD ["start-dev", "--import-realm"]
